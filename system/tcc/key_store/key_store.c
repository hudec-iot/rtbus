/* SPDX-License-Identifier: MPL-2.0 */
/* Shared host public-key HEX writer; included by the packer and J-Link helper. */
#include "key_store.h"

static int write_ihex_record(FILE *fp, uint8_t type, uint16_t address,
                             const uint8_t *data, uint8_t len)
{
    uint8_t sum = len + (uint8_t)(address >> 8) + (uint8_t)address + type;

    if (fprintf(fp, ":%02X%04X%02X", len, address, type) < 0) {
        return -1;
    }

    for (uint8_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + data[i]);
        if (fprintf(fp, "%02X", data[i]) < 0) {
            return -1;
        }
    }

    if (fprintf(fp, "%02X\n", (uint8_t)(0U - sum)) < 0) {
        return -1;
    }

    return 0;
}

static int key_hex_contents(FILE *fp, uint32_t address, const uint8_t public_key[65])
{
    uint32_t upper = UINT32_MAX;
    unsigned offset;
    int failed = 0;
    for (offset = 0; offset < RTBUS_KEY_PUBLIC_SIZE && !failed; offset += 16) {
        uint8_t bank[2];
        unsigned size = RTBUS_KEY_PUBLIC_SIZE - offset;
        uint32_t current = address + offset;
        if (size > 16) size = 16;
        if ((current >> 16) != upper) {
            upper = current >> 16;
            bank[0] = (uint8_t)(upper >> 8); bank[1] = (uint8_t)upper;
            failed = write_ihex_record(fp, 4, 0, bank, 2);
        }
        if (!failed) failed = write_ihex_record(fp, 0, current & 65535, public_key + offset, size);
    }
    if (!failed) failed = write_ihex_record(fp, 1, 0, NULL, 0);
    return failed;
}

static int write_key_hex(const char *path, uint32_t address, const uint8_t public_key[65])
{
    int failed;
    FILE *fp = fopen(path, "w");
    if (!fp) { fprintf(stderr, "Cannot write key HEX: %s\n", path); return -1; }
    failed = key_hex_contents(fp, address, public_key);
    if (fclose(fp)) failed = -1;
    return failed;
}

#ifdef RTBUS_KEY_DERIVE_ONLY
static int key_hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

/* J-Link permits runtime/default-key HEX data, but never application writes. */
static int validate_runtime_hex(const char *path, uint64_t limit)
{
    FILE *fp = fopen(path, "r");
    char line[1024];
    uint32_t base = 0;
    int eof = 0, data_seen = 0, valid = fp != NULL;
    if (!fp) { fprintf(stderr, "Cannot open runtime HEX: %s\n", path); return -1; }
    while (valid && fgets(line, sizeof(line), fp)) {
        uint8_t row[260];
        size_t length = strcspn(line, "\r\n");
        size_t i, bytes;
        unsigned sum = 0, count, type, address;
        if (!length) continue;
        if (eof || line[0] != ':' || length < 11 || !(length & 1) ||
            length > 1 + 2 * sizeof(row)) { valid = 0; break; }
        bytes = (length - 1) / 2;
        for (i = 0; i < bytes; ++i) {
            int hi = key_hex_digit(line[1 + 2 * i]);
            int lo = key_hex_digit(line[2 + 2 * i]);
            if (hi < 0 || lo < 0) { valid = 0; break; }
            row[i] = (uint8_t)((hi << 4) | lo);
            sum += row[i];
        }
        if (!valid || (sum & 255) || bytes != (size_t)row[0] + 5) {
            valid = 0; break;
        }
        count = row[0]; type = row[3]; address = (row[1] << 8) | row[2];
        if (type == 0) {
            if (count) data_seen = 1;
            if (count && (uint64_t)base + address + count > limit) valid = 0;
        } else if (type == 2 || type == 4) {
            if (count != 2 || address) { valid = 0; break; }
            base = ((uint32_t)row[4] << 8 | row[5]) << (type == 2 ? 4 : 16);
        } else if (type == 1) {
            if (count || address) valid = 0;
            eof = 1;
            continue;
        } else if ((type != 3 && type != 5) || count != 4 || address) valid = 0;
    }
    if (ferror(fp)) valid = 0;
    fclose(fp);
    if (!valid || !eof || !data_seen) {
        fprintf(stderr, "Invalid runtime HEX or data outside permitted runtime/key range: %s\n", path);
        return -1;
    }
    return 0;
}

#endif /* RTBUS_KEY_DERIVE_ONLY */
