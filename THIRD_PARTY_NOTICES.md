# Third-party software

RTBus-owned source uses MPL-2.0 unless a file states otherwise. Third-party
software retains its own license; the root MPL license does not relicense it.
This inventory identifies dependencies, not a complete binary license audit.

| Component | Use / version source | License and source information |
|---|---|---|
| Zephyr | Firmware; `ZEPHYR_VERSION` in `tools/docker/docker.mk` (default v4.4.0) | Apache-2.0 for the project; individual files and modules may differ. https://github.com/zephyrproject-rtos/zephyr |
| MCUboot | Bootloader; revision selected by the Zephyr west manifest | Apache-2.0 with separately licensed bundled code. https://github.com/mcu-tools/mcuboot/blob/main/LICENSE |
| Zephyr modules / HALs | Firmware; revisions from the resolved west workspace | Each module's own LICENSE and file notices apply. Preserve the resolved manifest and the modules used by each firmware build. |
| Zephyr SDK / arm-zephyr-eabi | Builder and separately distributed Windows toolchain ZIP | Mixed licenses for GCC, binutils, libraries and other components; do not label the entire SDK MPL. https://github.com/zephyrproject-rtos/sdk-ng |
| Arduino CLI | Builder; `ARDUINO_CLI_VERSION` (default 1.5.1) | GPLv3; retain the license and corresponding source/build information. https://github.com/arduino/arduino-cli/blob/master/LICENSE.txt |
| TinyCC / TCC | Builder OS package and external Windows ZIP | LGPL licensing with component-specific exceptions/notices; use the exact distribution's COPYING and source. https://github.com/TinyCC/tinycc/blob/mob/COPYING |
| micro-ecc | Host P-256 signing; vendored revision and changes in `system/tcc/crypto/README.md` | BSD-2-Clause; `system/tcc/crypto/micro-ecc/LICENSE.txt`; https://github.com/kmackay/micro-ecc |
| Brad Conte SHA-256 | Host SHA-256; source hashes and changes in `system/tcc/crypto/README.md` | Public domain per upstream README; https://github.com/B-Con/crypto-algorithms |
| GitHub CLI | Builder OS package | MIT for the project, plus dependency notices. https://github.com/cli/cli |
| embeddedcontainers Zephyr image | Builder base; `ZEPHYR_BASE_IMAGE` | A collection of software under individual licenses. Retain base-image notices and record its digest. https://github.com/embeddedcontainers |
| Linux OS packages and Python dependencies | Builder; Dockerfile install lists | Package-specific licenses and transitive dependencies. Preserve installed notices and corresponding-source mechanisms where applicable. |

## Distribution boundaries

The SDK archive contains prebuilt firmware as well as RTBus source. The public
repository provides runtime integration source separately. Use SOURCE_INFO.md
to locate source and to record the build-specific provenance of each binary.

The Windows toolchain and TCC ZIP URLs in the package index are separate
artifacts. An index link does not replace license files or corresponding-source
information inside or accompanying those distributions. Their actual archive
contents have not been audited here.

Container images redistribute their installed software. Project OCI labels
identify RTBus metadata, not a common license for every included component.

## Provenance still requiring confirmation

Before assigning a new file-level license, confirm the origin and retained
upstream notices of these existing unmarked files:

- zephyr/modules/rtbus/lib/bmalloc.c
- zephyr/modules/rtbus/lib/linklist.c
- zephyr/runtime/src/platform/nordic/nordic_hv_regout0_3v3.c

They have not been relabeled by the license cleanup. The generic root notice
alone does not establish rights to any third-party material in these files.
