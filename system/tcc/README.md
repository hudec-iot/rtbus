# TCC host tools

This directory groups the portable C host tools used by the RTDuo Arduino
package. These sources run on the development computer; they are not linked
into the MCU runtime or native application.

| Source | Purpose | Callers |
| --- | --- | --- |
| `header.c` | Application image packing, signing, and public-key derivation | Arduino pack recipe, native `system/make/core.mk`, `system/secure/` key-generation scripts |
| `jlink_runtime.c` | Runtime/public-key HEX checks and J-Link installation | Arduino Burn Bootloader recipes using `tcc -run` |
| `ymodem_upload.c` | Signed application upload over serial | Arduino upload/program recipes using `tcc -run` |
| `key_store/key_store.c` | Shared public-key Intel HEX writer and runtime HEX checks | Included by the packer and J-Link helper |
| `crypto/` | Host P-256/SHA-256 implementation, sources, and licenses | Included by the packer and J-Link helper |

The Arduino package uses the SDK TCC. Native application builds compile the
host packer with `HOST_CC` (default `cc`); the Unix key-generation script also
supports a host C compiler fallback. TCC itself is an external tool dependency,
not vendored in this directory.

`system/secure/generate-keys.sh` and `system/secure/generate-keys.bat` compile
`system/tcc/header.c` and write keys beside the scripts by default. The complete `system/` tree is already
included by local Arduino staging and release packaging, so the new directory
and dependency licenses travel with the SDK.

Tool entry sources include shared ABI headers from `../framework/` and keep
their crypto/key_store implementation includes relative to this directory. The key
helper uses `../../framework/include/runtime_macro.h` for the public-key size.
Runtime/application sources continue to use framework headers and do not include
these host-only implementations.
