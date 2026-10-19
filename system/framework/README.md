# RTBus application framework

This directory contains the shared runtime ABI contract and application-side
API wrappers used by RTDuo Arduino sketches and standalone native applications.

- `abi/include/`: ABI magic, version, table slots, event identifiers, and
  request data layouts shared by the application and resident runtime.
- `include/`: application API declarations and Serial, BLE, and Diagnostics
  wrappers. `runtime_api.h` is the application umbrella header.

Applications include both directories. The Arduino platform and native GCC
build rules provide these include paths; Zephyr builds use the same contract.
Header names, function symbols, slot numbers, and data layouts are preserved.

The ABI call thunks and application entry remain in `system/startup.S`.
Arduino adapters in `cores/rak4631/`, `cores/rak3172p/`, and `cores/rak3172t/`
include the shared headers and implementations under `system/arduino/`.
RAK4200 retains its existing adapters. Runtime API implementations,
ABI table installation, and application loading remain in
`zephyr/modules/rtbus/services/native_service/`.

Completion storage belongs to application RAM. Existing wrapper waiting and
callback behavior is unchanged by this directory organization.

The Arduino package includes the entire `system/` directory, so these headers
are distributed with the platform without a separate packaging step.

Arduino cores use `system/arduino/arduino_compat.h` for basic types,
constants, bit operations, math helpers, and ASCII character classification.
Shared application implementations for `map()`, pseudo-random numbers, and
Serial number formatting live under `system/arduino/` and are included by the
board adapters. The former `include/arduino_compat.h` path forwards to the
shared header. They do not require an application heap or libc formatter.

Serial supports character output, integer bases (`DEC`, `HEX`, `OCT`, `BIN`),
and `print/println(double, digits)` with two decimal places by default.
Fixed-decimal floating output rounds to the requested precision and reports
`nan`, `inf`, or `ovf` for special or out-of-range values. Its supported finite
magnitude is at most 4294967040, matching the usual Arduino Print limit.
This is independent of runtime `Serial.printf()` formatting, whose existing
160-byte buffer limits each call to 159 output bytes.

The runtime baseline enables `CONFIG_CBPRINTF_FP_SUPPORT` for floating-point
formats in the Serial printf and RTT printk paths. This enables formatting;
it does not select a hardware floating-point ABI or enable hardware FPU use.

Application compilation uses function/data sections and links with
`--gc-sections`, so unused helpers and their private data can be discarded.
Runtime formatting support is part of the resident firmware and is budgeted
separately. The shared Arduino layer now provides Print/Stream/String and
GPIO interrupt adapters. ADC/PWM and complete application math-library
integration remain separate work. See `system/arduino/README.md` for supported
operations, application String storage, and driver limitations.
