# Agent Instructions

## Project

This repository is the RTBus RTDuo Arduino platform.

RTDuo is the Arduino-compatible application profile for RTBus. It builds small
Arduino-style application images that communicate with a resident RTBus runtime
firmware through the runtime ABI.

The Arduino package identity is:

```text
rtbus:rtduo:<board>
```

Example:

```text
rtbus:rtduo:RAK4631
```

## Current Focus

The main ongoing work is integrating the application service into the RTBus
module layer. Keep this direction in mind when changing runtime/application
boundaries, build rules, ABI-facing code, or module structure.

The goal is to form RTBus as a Zephyr subsystem. The Zephyr runtime application
should stay thin, while RTBus-owned runtime behavior lives under
`zephyr/modules/rtbus/`. Do not move RTBus runtime loop or native
service bootstrap logic back into `zephyr/runtime/src/main.c`.

Current RTBus subsystem split:

- `services/rtbus_runtime.c`: owns the static Zephyr thread that calls
  `rtbus_init()` and runs the `rtbus_process()` loop.
- `services/rtbus_diagnostics.c`: owns the diagnostics task. This task uses a
  `switch (ctx->id)` handler and currently starts the native service from its
  `RTBUS_SYS_INIT` case.
- `services/native_service/`: owns native application loading, ABI table setup,
  and the native application thread.

Expected boot flow:

1. Zephyr starts the static RTBus runtime thread from `rtbus_runtime.c`, then
   the thread calls `rtbus_init()`.
2. EMOS/RTBus posts `RTBUS_SYS_INIT`; `rtbus_diagnostics.c` receives that event
   and calls `native_service_start()`.
3. The native service creates/runs the native application thread.

The runtime app `main.c` is intentionally minimal and may only return `0`.

`dummy_service.c` and `rtbus_bootstrap.c` have been removed. Runtime startup is
currently represented by the diagnostics task.

### RTBus Task Addition Rule

Use the diagnostics service pattern as the default maintenance rule for adding
future RTBus task event handling.

- Implement task handlers with a `switch (ctx->id)` and add each supported event
  as an explicit `case`.

## Main Areas

- `cores/`, `variants/`, `libraries/`, `system/`: Arduino platform sources.
- `zephyr/`: RTBus Zephyr runtime integration.
- `zephyr/runtime/`: RTBus runtime firmware.
- `zephyr/bootloader.mk`: MCUboot build wrapper.
- `bootloader/mcuboot/`: west-provided MCUboot source.
- `Makefile`: root entry for Arduino application, runtime, and bootloader builds.
- `system/make/`: shared project build fragments.
- `tools/docker/`, `tools/scripts/`: host-side container and packaging tools.
- `tools/scripts/package_arduino.mk`: isolated Arduino package staging and release
  archive/index rules; included by the root Makefile.
- `system/make/core.mk`: pure native GCC compilation, linking, and image packing
  rules; do not add container orchestration here.
- `cores/<profile>/Makefile`: standalone native application entry; starts the
  builder container and invokes `system/make/core.mk` inside it.
- `tools/docker/docker.mk`: shared container settings and execution parameters.
- `system/framework/abi/include/`: runtime/application ABI contract.
- `system/framework/include/`: application API declarations and wrappers.
- `system/startup.S`: application entry and ABI call thunks.
- `system/header.c`: host-side application image packer; keep it in the build flow.
- `cores/<profile>/main.c`: Arduino and standalone native application entry,
  selected by the standard Arduino `ARDUINO` macro.
- `bootloader/`, `modules/`: upstream or vendored platform pieces.
- `.arduino/package/`: local Arduino SDK staging.
- `<sketch>/build/<BOARD>/`: Arduino application build output.
- `cores/<profile>/firmware/build.*/`: runtime and bootloader build output.
- `cores/<profile>/build/`: standalone native application build output.

## Active Board Profiles

The profiles currently selected by `system/make/boards.mk` are:

- `rak4631`
- `rak3172p`
- `rak3172t`
- `rak4200`

Use `BOARD_PROFILE=<profile>` for root board-specific commands. Native builds
select the profile through the Makefile in each core directory.

## Common Commands

Build the Podman builder image:

```sh
make docker.build
```

List Arduino boards:

```sh
make arduino.boards
```

Compile the default Arduino sketch:

```sh
make application BOARD_PROFILE=rak4631
```

Build Zephyr runtime and bootloader:

```sh
make runtime BOARD_PROFILE=rak4631
make bootloader BOARD_PROFILE=rak4631
```

Build a standalone native C application through its core entry:

```sh
cd cores/rak4631
make
```

Zephyr GCC runs inside the builder container. Native output defaults to the
core directory's `build/`; `make clean` there removes that output. Set `VM`
and `DOCKER_IMAGE` to select the container CLI and builder image.
Root `make application` and `make application.clean` always use Arduino.
Do not reintroduce `application.gcc` or `APPLICATION_BACKEND` routing.

Inspect selected board profile variables:

```sh
make board.profile BOARD_PROFILE=rak4631
```

## Editing Guidance

- Keep changes focused on the requested RTBus/RTDuo behavior.
- Prefer existing Makefile targets and board profile patterns over adding new
  build flows.
- Do not edit generated build output unless explicitly requested.
- Do not stage changes or update the git index. Leave all edits in the working
  tree unless the user explicitly asks to run `git add` or otherwise change
  staged state.
- Preserve the user's existing staged changes exactly. Do not add to, unstage,
  refresh, replace, or otherwise mutate the staged snapshot unless the user
  explicitly asks to change staged state.
- Do not use git commands to change file content or to "fix" index/staged
  state. In particular, do not run `git restore --staged`, `git reset`,
  `git checkout --`, `git add`, `git commit`, or similar index/worktree
  mutation commands unless the user explicitly requests that exact operation.
  If staged state looks wrong or surprising, report it and wait for the user's
  direction instead of correcting it automatically.
- Treat upstream or vendored trees such as `zephyr/`, `modules/`, and MCUboot
  code as higher risk; keep changes there minimal and justified.
- Preserve the public package identity `rtbus:rtduo`.
- Low-level ABI names have been migrated to RTBus naming; do not reintroduce
  legacy names for compatibility unless explicitly requested.
- If a name is ABI-facing or shared with native applications, verify the ABI
  impact before renaming it. Prefer internal RTBus/native naming only behind
  compatibility layers.
- `native_service` naming is intentional for the runtime-side service that
  loads and runs the native application.
- ABI-facing native API names should use `RTBUS_API_*`.
- The native application owns its own RAM region. Avoid adding Zephyr heap usage
  unless there is a concrete need.

## Verification

Do not run build, compile, flash, or other verification commands unless the user
explicitly asks for them. When verification is not run, report that clearly in
the final response.

Use the existing board profile build targets. The usual smoke test for runtime
changes is:

```sh
make runtime BOARD_PROFILE=rak4631
```

If build output is needed, artifacts are exported under:

```text
cores/<profile>/firmware/build.runtime/
cores/<profile>/firmware/runtime.signed.hex
```
