# Shared RTDuo Arduino API

RAK4631, RAK3172P, and RAK3172T use this directory as their common
application-side Arduino implementation. Their files in `cores/<profile>/`
include the matching shared header or source, so Arduino continues to discover
and compile sources through its selected core. Do not also compile the shared
sources as a separate source set: that would duplicate definitions.

- `Arduino.h`, `RTDuo.h`, `HardwareSerial.h`: public API headers.
- `arduino_compat.h`: basic types, constants, bit/math helpers, and character utilities.
- `wiring_time.c`, `wiring_digital.cpp`: adapters for runtime ABI calls.
- `wiring_math.cpp`: application-local math and pseudo-random helpers.
- `Print.h`, `Print.cpp`, `Printable.h`: common output interface and formatting.
- `Stream.h`, `Stream.cpp`: timed input, parsing, and pattern searches.
- `WString.h`, `WString.cpp`, `String.h`: application-owned strings.
- `HardwareSerial.cpp`: common Serial/Serial1 implementation, derived from Stream.
- `lifecycle.cpp`, `main.c`: Arduino setup/loop adapters and application entry.

Board profiles, linker layouts, variants, and runtime configuration remain
board-owned. Native C example entry points remain in the core's `main.c`,
selected when `ARDUINO` is not defined. Runtime ABI implementations and GPIO
mapping remain runtime-owned. The additional clock, Serial control, and GPIO
interrupt APIs require ABI version 14. Rebuild both runtime and applications.

RAK4200 has not migrated to the shared core adapters. Its existing references
to the shared math and formatting helpers continue to work. The framework
compatibility header remains a forwarding include for its existing header path.

The complete `system/` tree is already included in Arduino staging and release
packages. Application function/data section garbage collection still applies.

## Supported Core Operations

- GPIO read/write/configure, including open-drain output where supported.
- `millis`, `micros`, both delay functions, `yield`, `shiftIn`, `shiftOut`,
  `pulseIn`, and `pulseInLong`.
- `attachInterrupt`, `attachInterruptParam`, `detachInterrupt`, and identity
  `digitalPinToInterrupt` mapping for runtime-exported GPIO identifiers.
- Print output for characters, strings, F literals, Printable objects, integer
  bases including 64-bit values, and fixed-decimal floating point.
- Stream timeouts, readBytes/readBytesUntil, parseInt/parseFloat, find/findUntil,
  and readString/readStringUntil.
- String constructors, copy/move/concatenation, comparisons, substring/search,
  replacement/removal, case/whitespace operations, and numeric conversions.
- Serial baud/framing selection, RX byte counts, peek/read, end, connection
  checks, and hardware-UART TX drain through the runtime interrupt driver API.

`serial_print.cpp` remains for RAK4200's legacy adapter. The migrated cores use
`Print.cpp` instead; do not compile the legacy formatting file separately.

## Resource and Driver Limits

String defaults to a 1024-byte application pool plus 128 bytes of allocation
metadata. Override `RTDUO_STRING_POOL_SIZE` through compiler flags if needed.
String never borrows Zephyr heap memory. The pool is shared by strings within
one application, is not ISR-safe, and is discarded when unused by linker GC.
Allocation failure is observable through reserve/concat and String validity;
string operations cannot grow past the pool's available contiguous space.

Global C++ constructors run before setup. Global destructors are not registered:
applications run until upgrade resets their RAM rather than exiting through libc.

`micros` uses the runtime system cycle counter with wrap extension. Its resolution
is the hardware clock resolution, not necessarily one microsecond. The existing
RAK4631 configuration uses a 32768 Hz system counter (about 30.5 us resolution).
Both pulse measurement APIs use this same clock and GPIO polling. Long IRQ masks,
clock suspension, and tickless/power-management behavior require target validation.

Interrupt callbacks run in ISR context and must not block, use Serial/Stream,
or modify String storage. The runtime disables registered callbacks before
discarding application code during upgrade. Check `arduinoLastError()` for
failures from void GPIO/interrupt APIs, including unsupported trigger modes.

Serial `availableForWrite()` returns zero: the application has no asynchronous
TX buffer. Hardware UART flush has a one-second timeout and depends on driver
TX-complete support. USB CDC flush reports `-ENOTSUP` via `Serial.lastError()`;
the UART facade has no portable host-delivery/drain operation. Framing support
also depends on the selected driver. `printf` retains its 159-byte output limit.

ADC/PWM, tone/noTone, Wire/SPI, and complete application libc/libm integration
remain separate work requiring board/peripheral decisions. No placeholder
success implementations are provided for these missing capabilities.

The additions have not been compiled or tested on hardware in this change.
