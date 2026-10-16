# Application signing

The SDK signs application images with ECDSA P-256/SHA-256. The host packer uses
vendored micro-ecc and SHA-256 C source compiled directly by TCC. No separately
installed signing tool or crypto library is required for an Arduino IDE
application build. Runtime uses Zephyr PSA Crypto.

## Reference keys and builds

The SDK includes an intentionally public reference pair:

- `application-p256-private.key`: 64 hexadecimal digits representing a 32-byte,
  big-endian P-256 private scalar; trailing whitespace is accepted.
- `application-p256-public.pub`: 65 binary bytes, SEC1 uncompressed format
  (`0x04 || X[32] || Y[32]`). This public key is embedded in runtime.

The root files are included in both local Arduino staging and release archives.
This guide is distributed as `APPLICATION_SIGNING.md` at the SDK root.
The Arduino recipe explicitly passes the SDK private-key path, so it does not
rely on the working directory or an environment variable. The packer checks the
key range and verifies its own signature before writing output.

The normal board targets use these files by default:

```sh
make runtime BOARD_PROFILE=rak4631
make application BOARD_PROFILE=rak4631
make runtime BOARD_PROFILE=rak3172t
make application BOARD_PROFILE=rak3172t
```

Standalone native applications continue to use their core entry:

```sh
make -C cores/rak4631
```

Override `RTBUS_SIGNING_KEY` for application builds and
`RTBUS_IMAGE_PUBLIC_KEY_FILE` for runtime builds to use another pair. Relative
container paths are resolved against the repository mount (`/workdir` by default).
For Arduino IDE/CLI, override the `build.application.signing.key` build property,
or set `RTBUS_SIGNING_KEY` before launching the IDE. An explicit nonempty
environment variable takes precedence over the recipe's SDK reference-key path.
Missing or malformed keys fail packaging without an unsigned fallback.

To derive the public key from a replacement private scalar, compile the same
packer with the SDK TCC and run:

```text
application_pack.exe --public-key replacement-private.key replacement-public.pub
```

Private-key generation remains separate from normal builds and requires a
cryptographically random scalar in the P-256 group range.
Replacing a key pair requires rebuilding both runtime and application.

The runtime target merges `zephyr/runtime/app-p256.conf` with the selected board
configuration. Direct Zephyr builds must merge the same fragment and pass
`-DRTBUS_IMAGE_PUBLIC_KEY_FILE=<absolute SEC1 public-key path>` to CMake.

## Image contract

```text
128-byte header (version 4) | payload | 64-byte signature (r || s)
<--------- SHA-256 signed bytes ---->
```

All integers in the header are little-endian. Signature components are fixed
32-byte big-endian integers, not ASN.1 DER. The shared constants are in
`system/framework/abi/include/rtbus_image_format.h`.

| Offset | Field | Meaning |
| --- | --- | --- |
| 4 | header_version, u16 | 4 |
| 6 | header_size, u16 | 128 |
| 8 | payload_size, u32 | Excludes header and signature |
| 12 | payload_crc32, u32 | Payload CRC32 |
| 100 | flags, u32 | Native format, signature algorithm/size, ABI |
| 104 | native metadata, 24 bytes | Existing entry/data/bss metadata |

Flags use bit 0 for native format, bit 1 for signed content, bits 8..15 for the
algorithm (`2` = ECDSA P-256/SHA-256), bits 16..23 for signature length in 4-byte
words (`16` = 64 bytes), and bits 24..31 for the native ABI. Other bits must be
zero. All flags and metadata are authenticated. Earlier image formats are
rejected. The linker and application execution address stay the
same. Both signed BIN and HEX contain the signature, and the packer reserves the
trailer space when checking slot capacity.

Host signing uses deterministic nonces following RFC 6979. The vendored source
notes describe the local corrections for standard hash reduction and byte order.
The host configuration uses portable 32-bit C arithmetic, disables other curves
and platform RNG code, and requires no Windows CryptoAPI linkage.

Runtime verifies before committing uploaded images and before loading an
application. A configuration with verification disabled refuses loading and
update commits. Boards without a staging partition write directly into the
application slot; rejected uploads cannot preserve the previous application.
Signature verification does not implement rollback protection.

## Resource budget

Runtime hashes flash in 128-byte chunks. Initial ECDSA workspace allocation is
8 KiB of Mbed TLS allocator memory, separate from application RAM and the system
heap. The runtime and system workqueue stack budgets are 4 KiB. Actual peak
allocation, stack use, and board verification latency require hardware measurement.

