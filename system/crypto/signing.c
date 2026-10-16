/* SPDX-License-Identifier: MPL-2.0 */
/* Included by header.c so the existing single-source TCC recipe stays portable. */
#include <ctype.h>
#include <limits.h>

/* Avoid compiler assembly/int128 and OS crypto library dependencies. */
#define uECC_PLATFORM 0
#define uECC_WORD_SIZE 4
#define uECC_SUPPORTS_secp160r1 0
#define uECC_SUPPORTS_secp192r1 0
#define uECC_SUPPORTS_secp224r1 0
#define uECC_SUPPORTS_secp256r1 1
#define uECC_SUPPORTS_secp256k1 0
#define uECC_SUPPORT_COMPRESSED_POINT 0
#define _UECC_PLATFORM_SPECIFIC_H_
#include "micro-ecc/uECC.c"
#include "sha256/sha256.c"

typedef char rtbus_sha256_word_must_be_32_bits[sizeof(WORD) == 4 && CHAR_BIT == 8 ? 1 : -1];

struct rtbus_sign_hash {
    uECC_HashContext interface;
    SHA256_CTX sha;
    uint8_t scratch[128];
};

static void rtbus_hash_init(const uECC_HashContext *base)
{
    struct rtbus_sign_hash *ctx = (struct rtbus_sign_hash *)base;
    sha256_init(&ctx->sha);
}

static void rtbus_hash_update(const uECC_HashContext *base,
                              const uint8_t *data, unsigned size)
{
    struct rtbus_sign_hash *ctx = (struct rtbus_sign_hash *)base;
    sha256_update(&ctx->sha, data, size);
}

static void rtbus_hash_finish(const uECC_HashContext *base, uint8_t *digest)
{
    struct rtbus_sign_hash *ctx = (struct rtbus_sign_hash *)base;
    sha256_final(&ctx->sha, digest);
}

static int rtbus_read_private_key(const char *path, uint8_t key[32])
{
    FILE *fp = fopen(path, "rb");
    size_t digits = 0U;
    int ch;

    memset(key, 0, 32);
    if (fp == NULL) {
        fprintf(stderr, "cannot open P-256 key: %s: %s\n", path, strerror(errno));
        return -1;
    }
    while ((ch = fgetc(fp)) != EOF) {
        unsigned value;

        if (isspace((unsigned char)ch)) {
            continue;
        }
        if (digits >= 64U || !isxdigit((unsigned char)ch)) {
            fclose(fp);
            fprintf(stderr, "P-256 private key must contain exactly 64 hex digits\n");
            return -1;
        }
        value = ch <= '9' ? (unsigned)(ch - '0') :
                (unsigned)(tolower((unsigned char)ch) - 'a' + 10);
        key[digits / 2U] = (uint8_t)((key[digits / 2U] << 4U) | value);
        ++digits;
    }
    ch = ferror(fp);
    fclose(fp);
    if (digits != 64U || ch) {
        fprintf(stderr, "invalid P-256 private key length or read error\n");
        return -1;
    }
    return 0;
}

static int rtbus_public_key(const uint8_t key[32], uint8_t public_key[65])
{
    public_key[0] = 0x04;
    if (!uECC_compute_public_key(key, public_key + 1, uECC_secp256r1())) {
        fprintf(stderr, "invalid P-256 private scalar\n");
        return -1;
    }
    return 0;
}

static int sign_component(const uint8_t *header, const uint8_t *payload,
                          size_t payload_size, const char *path,
                          uint8_t signature[RTBUS_IMAGE_P256_SIGNATURE_SIZE])
{
    uint8_t key[32];
    uint8_t public_key[65];
    uint8_t digest[32];
    SHA256_CTX sha;
    struct rtbus_sign_hash hash;

    if (rtbus_read_private_key(path, key) != 0 ||
        rtbus_public_key(key, public_key) != 0) {
        return -1;
    }
    sha256_init(&sha);
    sha256_update(&sha, header, RTBUS_IMAGE_HEADER_SIZE);
    sha256_update(&sha, payload, payload_size);
    sha256_final(&sha, digest);
    memset(&hash, 0, sizeof(hash));
    hash.interface.init_hash = rtbus_hash_init;
    hash.interface.update_hash = rtbus_hash_update;
    hash.interface.finish_hash = rtbus_hash_finish;
    hash.interface.block_size = 64;
    hash.interface.result_size = 32;
    hash.interface.tmp = hash.scratch;
    if (!uECC_sign_deterministic(key, digest, sizeof(digest), &hash.interface,
                                 signature, uECC_secp256r1()) ||
        !uECC_verify(public_key + 1, digest, sizeof(digest), signature, uECC_secp256r1())) {
        fprintf(stderr, "P-256 signing or signature self-check failed\n");
        return -1;
    }
    return 0;
}
