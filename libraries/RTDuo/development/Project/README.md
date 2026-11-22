# Project Development

Use `Project.ino` as a starting point for an RTDuo application. This development
sketch lives outside `examples/` and does not appear in the Arduino IDE examples
menu. It is the default sketch for `make application`.

From this directory, run `make compile BOARD_PROFILE=rak4631` to build the
sketch through the repository Makefile. Use `make boards` or `make version`
to inspect the local Arduino setup.

The default loop echoes serial input and prints an RTBus diagnostics result
every two seconds. The BLE peripheral sketch is available in
[`examples/BLE/Peripheral`](../../examples/BLE/Peripheral/Peripheral.ino).

Repository builds write directly to `Project/build/<BOARD>/`, for example
`Project/build/RAK4631/`. `make jlink.download.application` uses the same directory.
`make application.clean` removes the selected board's output; `make arduino.clean`
removes the selected sketch's entire build directory and local SDK staging.

When working in the SDK repository, uncomment one project include in
`Project.ino`. Existing project sources define `PROJECT_ALONE` and provide
their own `setup()` and `loop()`, replacing the default entry points.

For an independent application repository, copy this sketch into a directory
with the same name as its main `.ino` file, remove the SDK repository include
comments and `PROJECT_ALONE` guard, and implement `setup()` and `loop()`.
The `project/` sources are not included in the installed SDK package.

With the RTDuo SDK installed, build a standalone `Project` sketch using:

```sh
arduino-cli compile --fqbn rtbus:rtduo:RAK4631 Project
```
