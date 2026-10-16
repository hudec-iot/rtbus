# RTBus RTDuo Arduino Platform

RTDuo is the Arduino-compatible application profile for RTBus. It builds small,
signed native application images that run against a resident Zephyr/RTBus runtime
through the RTBus ABI.

The runtime owns board integration and services. Application logic can be updated
independently through YMODEM; capabilities exposed to applications are defined by
the runtime ABI. See [Architecture and Design](https://github.com/hudec-iot/rtbus/wiki/Architecture-and-Design)
for the design tradeoffs and comparisons with Arduino Core on Zephyr and LLEXT.

## Arduino IDE Installation

Add this URL under **File > Preferences > Additional Boards Manager URLs**:

```text
https://github.com/hudec-iot/rtbus/releases/download/package-index/package_rtbus_index.json
```

Open **Tools > Board > Boards Manager**, install the RTBus RTDuo package, and
select your board. The package identity is `rtbus:rtduo:<board>`, for example
`rtbus:rtduo:RAK4631`.

The SDK includes public examples, a TCC-based P-256 signing tool, and a reference
key pair. Install the runtime through **Tools > Burn Bootloader** using J-Link;
application updates use **YMODEM Application Upgrade**. See
[Upload and Runtime Installation](https://github.com/hudec-iot/rtbus/wiki/Upload-and-Runtime-Installation)
for setup and connection details.

## Build from Source

Current active board profiles are `rak4631`, `rak3172p`, `rak3172t`, and `rak4200`.
Repository builds use Docker or Podman with the published builder image selected
in `system/make/docker.mk`; it is pulled automatically when missing.

Build the default development application and the runtime:

```sh
make application BOARD_PROFILE=rak4631
make runtime BOARD_PROFILE=rak4631
```

Build the public HelloWorld example:

```sh
make application BOARD_PROFILE=rak4631 \
  ARDUINO_SKETCH=libraries/RTDuo/examples/HelloWorld
```

Build a standalone native C application:

```sh
make -C cores/rak4631
```

The default development sketch stays in the repository and is excluded from SDK
packages. See [Build and Board Profiles](https://github.com/hudec-iot/rtbus/wiki/Build-and-Board-Profiles)
for configuration, output locations, container maintenance, and Flash layouts.

## Documentation

The [RTBus wiki](https://github.com/hudec-iot/rtbus/wiki) contains the detailed guides:

- [Architecture and Design](https://github.com/hudec-iot/rtbus/wiki/Architecture-and-Design):
  resident runtime model, ABI boundaries, and comparisons.
- [Build and Board Profiles](https://github.com/hudec-iot/rtbus/wiki/Build-and-Board-Profiles):
  Arduino/native builds, board settings, and Flash layouts.
- [Runtime Architecture](https://github.com/hudec-iot/rtbus/wiki/Runtime-Architecture):
  subsystem ownership, boot flow, and Kconfig settings.
- [Upload and Runtime Installation](https://github.com/hudec-iot/rtbus/wiki/Upload-and-Runtime-Installation):
  J-Link, YMODEM, and runtime CLI commands.
- [Packaging and Repository Layout](https://github.com/hudec-iot/rtbus/wiki/Packaging-and-Repository-Layout):
  release workflows, SDK contents, and source locations.

[Application signing](zephyr/application-signing.md) documents P-256 keys and the
image format. The SDK includes that guide as `APPLICATION_SIGNING.md`.

## License

RTBus-owned source uses MPL-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE).
Third-party software retains its own licenses. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE_INFO.md](SOURCE_INFO.md)
for component information and firmware provenance. These documents are included
in SDK packages.
