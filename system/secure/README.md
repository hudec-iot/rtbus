# Reference keys and generation scripts

This directory contains the SDK reference P-256 key pair and its generation
entry points. The keys are moved without changing their contents.

| File | Purpose |
| --- | --- |
| `application-p256-private.key` | Reference application signing scalar, stored as hexadecimal text |
| `application-p256-public.pub` | Matching binary 65-byte SEC1 public key used by the runtime build |
| `generate-keys.sh` | Unix key-generation entry using TCC or a host C compiler |
| `generate-keys.bat` | Windows key-generation entry using PowerShell and TCC |

From the repository or installed SDK root:

```sh
./system/secure/generate-keys.sh [output-directory]
```

On Windows:

```bat
system\secure\generate-keys.bat [output-directory]
```

Both scripts resolve the SDK root from their own location and compile
`system/tcc/header.c`. Without an output-directory argument they write the pair
in this directory, backing up existing files under `.key-backup-*` first.
Explicit relative output directories remain relative to the caller's working
directory. The public-key file retains the `.pub` extension and binary format.

Default application signing and runtime public-key paths point into this
directory. `RTBUS_SIGNING_KEY`, `build.application.signing.key`, and
`RTBUS_IMAGE_PUBLIC_KEY_FILE` still allow explicit overrides. Local staging and
release packaging include these assets through the complete `system/` tree.
Both packaging paths exclude `.key-backup*` directories so previous key pairs
are not bundled with the reference pair.
Moving these files does not generate or rotate keys, rebuild firmware, or change
the key already installed on a device.
