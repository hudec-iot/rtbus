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

`sleep()` is an RTDuo Arduino extension declared in `Arduino.h`, wrapping
`rtbus_sleep()` and returning its negative errno on failure. Success never
returns. It takes no arguments and does not configure wakeup sources.
The Project development sketch uses this Arduino wrapper.

Sleep requests follow the native post queue -> `rtbus_post()` -> `emos_post()`
path. The diagnostics task handles the explicit `APPLICATION_DIAGNOSTICS_SLEEP`
case (also when optional diagnostics tests are disabled), calling the RTBus
power service in runtime thread context. Application-side `rtbus_sleep()` is
an inline post/result-wait wrapper with completion storage in application RAM.
It uses the normal result-wait callback and upgrade behavior; successful system
off never returns. There is no dedicated sleep thunk, slot, or semaphore.
Removing the slot moves the ADC slot to 23: rebuild runtime and applications
for ABI 18. Public `sleep()` and `rtbus_sleep()` signatures are unchanged.

`rtbus_sleep()` is an RTDuo extension available through `Arduino.h` or
`RTDuo.h`. It enters system deep sleep using Zephyr system off; success never
returns. It returns `-RTBUS_ENOTSUP` if the runtime lacks power-off support.
The runtime enables `CONFIG_POWEROFF` on supported SoCs. This uses system off,
not `k_sleep()` or scheduler-driven idle power management: Zephyr thread
timeouts do not resume the runtime, and no timed wakeup is requested.
Finish pending I/O before calling it. This initial API does not configure or
clear wake sources or flush output. On RAK4631, runtime preparation resets the
SX1262 and sends cold-start SetSleep with RTC wake disabled before system off,
so the radio and its TCXO can stop.
On RAK4631 only, P1.05 (RF_IO) is then driven low before system off; Reset
restores its output-high boot configuration. GPIO failures also prevent system
off and return a negative errno. SPI failures also prevent system off;
BUSY has a 100 ms timeout. This raw shutdown path
requires SPI and does not support an enabled LoRa driver (returns `-ENOTSUP`
to avoid concurrent radio access). Other external peripherals are not shut
down. Existing
hardware wake configuration can therefore still affect sleep behavior; this
does not guarantee that only Reset or a power cycle can restart the board.
Sleep requires ABI version 15; rebuild both runtime and applications.

Writable command characteristics require ABI version 16. Set
`RUNTIME_BLE_GATT_CHRC_WRITE` and a write permission when registering the
characteristic. Runtime accepts single writes of 1..32 bytes at offset zero,
queues `RUNTIME_EVENT_BLE_GATT_WRITE`, and leaves the characteristic value
unchanged. Decode with `runtime_event_is_ble_gatt_write()` in the application
event callback. The decoded data pointer is valid during that callback only.
Prepared writes are rejected; ATT success acknowledges event queue admission.
Rebuild both runtime and applications for ABI 16.

```cpp
void setup() {
    rtbus_sleep();
}

void loop() {
}
```

The complete `system/` tree is already included in Arduino staging and release
packages. Application function/data section garbage collection still applies.

## Supported Core Operations

### ADC (ABI 17)

`analogRead(A0)` / `analogRead(AIN1)` is implemented on RAK4631, using
P0.31 (SAADC AIN7). `A0` is application pin 20; numeric pin 0 is not an alias.
RAK4631 also exports `AIN0` for board battery sense (WB_A0, P0.05 / SAADC AIN3).
`analogRead(AIN0)` reads only the ADC input; battery divider compensation and
percentage conversion are not implemented. `A0` remains an alias of `AIN1`
for compatibility. AIN0 uses 40 us acquisition time for the divider input.
The runtime ADC map is separate from the GPIO map; only explicitly mapped
pin IDs are accepted, including AIN0's physical ID. Other profiles return
`-RTBUS_ENOTSUP` until their ADC maps and driver configurations are supplied.
An unmapped pin on an ADC-enabled profile returns `-RTBUS_EINVAL`.

The Arduino default is 10 bits (0..1023). `analogReadResolution(bits)` accepts
1..16 output bits. RAK4631 samples at 12 bits: lower output resolutions discard
low bits, and higher output resolutions pad zeros without increasing accuracy.
An invalid resolution causes subsequent reads to return `-RTBUS_EINVAL` until
a valid resolution is selected. `analogRead` returns negative errno on failure;
it does not update `arduinoLastError()`. Calls are synchronous and must run in
application thread context. No callbacks are dispatched during conversion.

RAK4631 uses a fixed internal 0.6 V reference, gain 1/6, and 10 us acquisition
time (nominal ADC full scale 3.6 V). This does not permit input above the GPIO
electrical limits or board supply. `analogReference()` is not implemented.
Reading disconnects the pin's digital output and pulls; use `pinMode()` again
before reusing that pin as digital GPIO. ADC requests are serialized in runtime,
and no Zephyr heap or application buffer is used.

```cpp
analogReadResolution(12);
int raw = analogRead(A0);
if (raw >= 0) {
    Serial.println(raw);
}
```

Rebuild both runtime and applications for ABI 17. ADC itself returns raw counts;
the HX711 application owns its RAK19007 battery divider and percentage conversion.

### Other operations

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

## Resource and Driver Limits

String defaults to a 1024-byte application pool plus 128 bytes of allocation
metadata. Override `RTDUO_STRING_POOL_SIZE` through compiler flags if needed.
String never borrows Zephyr heap memory. The pool is shared by strings within
one application, is not ISR-safe, and is discarded when unused by linker GC.
Allocation failure is observable through reserve/concat and String validity;
string operations cannot grow past the pool's available contiguous space.
Copy construction and assignment preserve explicit lengths, including embedded
NUL bytes. Failed `operator+` concatenation returns an invalid String instead
of a valid partial result. Floating-point String construction supports fixed
decimal output across the finite double range, with at most ten fractional
digits, independently of Print's integer overflow limit. Formatting remains
subject to floating-point precision and the available application String pool.

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
Flush failures also set the Print write-error flag; a successful begin clears
that flag. Native writes use the same runtime port state and write path as the
maintenance service. RX overflow discards newly arriving bytes while preserving
bytes already waiting to be read.

By default, Serial shares its port with the maintenance CLI. Only lines starting
with the reserved `@RTBUS:` header are consumed; other `@` lines are returned to
the application when the prefix mismatches. An incomplete matching prefix is
held until more bytes arrive. Set `CONFIG_RTBUS_RUNTIME_SERIAL_CLI_MUX=n` in
the runtime configuration for a fully transparent byte stream, including partial
headers; in-band CLI uploads are then unavailable. Serial1 has no CLI routing.
With multiplexing enabled, Serial.end() discards application RX data while
keeping the runtime maintenance receiver active.

PWM, tone/noTone, Wire/SPI, and complete application libc/libm integration
remain separate work requiring board/peripheral decisions. No placeholder
success implementations are provided for these missing capabilities.

The additions have not been compiled or tested on hardware in this change.
