# RTBus Zephyr Profiles

`BOARD_PROFILE` is the RTBus product-level board selection. It is not always
the same as the Zephyr board name.

Each `cores/<profile>/profile.mk` file owns the metadata that explains where
the Zephyr board support comes from:

- `PROFILE_NAME`: RTBus profile name.
- `ZEPHYR_BOARD`: board name passed to `west build -b`.
- `ZEPHYR_BOARD_SOURCE`: `upstream` for Zephyr-provided boards, `project` for
  boards provided by this repository.
- `ZEPHYR_BOARD_VARIANT_OF`: optional base profile or board when this profile is
  a hardware variant.
- `ZEPHYR_SOC`: SoC used by the Zephyr board.
- `JLINK_TARGET`: SEGGER J-Link target name.
- `BOARD_ROOTS`: optional Zephyr board root list. Project-owned boards should
  keep their board tree under the owning core's `zephyr/boards/` directory.

Runtime, profile, and project-owned Zephyr board files live under
the owning `cores/<profile>` directory. The profile layer is the single place
for board identity and source mapping.

See [Application signing](application-signing.md) for P-256 application packaging,
runtime verification, keys, and initial resource budgets.

## Native application GPIO interrupts

`attachInterrupt()` and `attachInterruptParam()` register runtime-owned GPIO
interrupts. The hardware ISR only marks the pin pending and wakes the native
thread; application callbacks execute in that thread at event dispatch points
(delay, yield, service-result waits, and native entry boundaries). A busy
application must call `yield()` to allow dispatch. This is deferred notification,
not preemptive execution of application code in a software IRQ.

`noInterrupts()` pauses application event dispatch and `interrupts()` resumes
it. Neither masks kernel, BLE, USB or other runtime hardware interrupts.
Repeated GPIO triggers for the same pin coalesce into a single pending callback;
this interface does not preserve every edge or its timestamp. Callbacks do not
recursively dispatch other callbacks. Detaching a pin clears its pending event,
and stopping the native image removes its GPIO registrations before RAM reuse.

The API table slots and function signatures are unchanged. Callback context and
interrupt-control semantics differ from traditional Arduino ISR callbacks.
Code that needs uninterrupted bit-banging or exact edge timing must use an
appropriate runtime/hardware service instead of relying on `noInterrupts()`.
