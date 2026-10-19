# Source availability and binary provenance

RTBus RTDuo source is available under MPL-2.0 at:
https://github.com/hudec-iot/rtbus

For an SDK release `v<VERSION>`, its release tag identifies the platform source:
https://github.com/hudec-iot/rtbus/tree/v<VERSION>

Runtime integration is under `zephyr/modules/rtbus/` and `zephyr/runtime/`;
board configuration is under `cores/<profile>/zephyr/`. Shared ABI and
application wrappers are under `system/framework/`. These paths are in the
public source repository even when not included in the Arduino archive.

TCC-compatible host tool sources and their bundled signing dependencies are
under `system/tcc/`; their licenses and provenance are included in that tree.
Reference signing keys and generation scripts are under `system/secure/`.

Prebuilt HEX files are copied into SDK archives without rebuilding them.
Consequently the SDK tag does not by itself prove which commit built a HEX.
For each distributed firmware, retain its SHA256, RTBus build commit, board
profile/configuration, resolved `west manifest --freeze` output, local upstream
patches, SDK version and builder image digest. Make these records and the
corresponding sources available with that binary. Do not substitute the current
main branch or default dependency versions for the actual binary's provenance.

The container workflow initializes Zephyr and modules in a mounted workspace
volume. They are separate from the builder image. Their upstream sources and
license files remain governed by their own licenses; see THIRD_PARTY_NOTICES.md.

For container and toolchain distributions, retain the exact upstream source,
notices and build information required by their respective licenses. A generic
repository link alone is not a complete corresponding-source record.
