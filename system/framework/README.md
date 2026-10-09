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
Arduino adapters remain in `cores/<profile>/`. Runtime API implementations,
ABI table installation, and application loading remain in
`zephyr/modules/rtbus/services/native_service/`.

Completion storage belongs to application RAM. Existing wrapper waiting and
callback behavior is unchanged by this directory organization.

The Arduino package includes the entire `system/` directory, so these headers
are distributed with the platform without a separate packaging step.
