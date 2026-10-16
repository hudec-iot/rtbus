# Host signing dependencies

micro-ecc: https://github.com/kmackay/micro-ecc
Upstream revision: 541b3a78026420a3e369c4c9281c396b5e531113
Downloaded master archive SHA-256: a94c515d12a37676af27a920bf6cf31b2d8326d4090e98b422f2c3256b4db879
License: BSD-2-Clause; see micro-ecc/LICENSE.txt. Local changes are listed below; retain this license in SDK distributions.

SHA-256: Brad Conte, https://github.com/B-Con/crypto-algorithms
Source files: sha256.c and sha256.h; public domain as stated in upstream README.
Downloaded source SHA-256: 4cbc93d30c28cee2a056d6f52cbce6e66ff9450f1d712ee4f59761b9b4486fad
Downloaded header SHA-256: a946e621775fd8251624e77d19f25407c4b6074ab780027e5797b77c7acf79b5
Local portability changes are documented below.

The host wrapper selects portable 32-bit C arithmetic, P-256 only, and RFC 6979
signing. The upstream platform RNG module is disabled: signing does not require
OS-specific APIs or linked crypto libraries. Key generation is a separate step.

SHA-256 local changes: use standard string.h; cast bytes to unsigned WORD before
left shifts to avoid signed integer promotion overflow.

The selected P-256 deterministic-signing path in micro-ecc/uECC.c has two local
RFC 6979 corrections: reduce the message digest modulo the curve order for
bits2octets, and convert the generated nonce from big-endian bytes to native
words. These changes are covered by the RFC 6979 A.2.5 SHA-256 test vector.
Other curves are disabled in the host wrapper and are not validated here.
