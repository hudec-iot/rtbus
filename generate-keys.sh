#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# Generate a new P-256 reference/application key pair without OpenSSL.
set -euo pipefail
umask 077

if (( $# > 1 )); then
    echo "Usage: $0 [output-directory]" >&2
    exit 2
fi

sdk_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
output_dir=${1:-$sdk_root}
compiler=${TCC:-tcc}
if ! command -v "$compiler" >/dev/null 2>&1; then
    if [[ -n ${TCC:-} ]]; then
        echo "TCC compiler not found: $compiler" >&2
        exit 1
    fi
    compiler=cc
    if ! command -v "$compiler" >/dev/null 2>&1; then
        echo 'Set TCC to the TCC executable, or install a C compiler.' >&2
        exit 1
    fi
fi

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rtbus-keygen.XXXXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT
"$compiler" -x c -std=c99 -Wall -Wextra -Werror -O2 \
    -o "$work_dir/application_pack" "$sdk_root/system/header.c"

# The private scalar is 32 big-endian bytes, represented as 64 hex digits.
# --public-key validates its range and derives the matching SEC1 public key.
od -An -N32 -tx1 /dev/urandom | tr -d ' \n' > "$work_dir/private.key"
printf '\n' >> "$work_dir/private.key"
"$work_dir/application_pack" --public-key \
    "$work_dir/private.key" "$work_dir/public.pub"

mkdir -p -- "$output_dir"
output_dir=$(cd -- "$output_dir" && pwd)
private_key="$output_dir/application-p256-private.key"
public_key="$output_dir/application-p256-public.pub"
if [[ -e $private_key || -e $public_key ]]; then
    backup_dir=$(mktemp -d "$output_dir/.key-backup-XXXXXXXX")
    for key_file in "$private_key" "$public_key"; do
        if [[ -e $key_file ]]; then
            cp -p -- "$key_file" "$backup_dir/"
        fi
    done
    printf 'Previous keys saved in: %s\n' "$backup_dir"
fi
mv -- "$work_dir/private.key" "$private_key"
mv -- "$work_dir/public.pub" "$public_key"
printf 'Generated private key: %s\nGenerated public key: %s\n' "$private_key" "$public_key"
echo 'Rebuild the application, then use Burn Bootloader with J-Link to install runtime and the new public key.'
