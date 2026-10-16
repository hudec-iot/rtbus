/* SPDX-License-Identifier: MPL-2.0 */
#include "rtbus_image_verify.h"
#include <rtbus_image_format.h>

#include <errno.h>
#include <psa/crypto.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(runtime_key_partition)) +
             DT_REG_SIZE(DT_NODELABEL(runtime_key_partition)) <=
             DT_REG_ADDR(DT_NODELABEL(component_slot_partition)));

BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(runtime_key_memory)) ==
             DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) +
             DT_REG_ADDR(DT_NODELABEL(runtime_key_partition)));
BUILD_ASSERT(DT_REG_SIZE(DT_NODELABEL(runtime_key_memory)) ==
             DT_REG_SIZE(DT_NODELABEL(runtime_key_partition)));

/* Linked into the dedicated Flash region, never cached by the verifier. */
const uint8_t rtbus_default_public_key[]
    __attribute__((section(".rtbus_public_key"), used, aligned(4))) = {
#include "rtbus_image_public_key.inc"
};

static int read_public_key(uint8_t public_key[RTBUS_KEY_PUBLIC_SIZE])
{
    const struct flash_area *area;
    int rc = flash_area_open(PARTITION_ID(runtime_key_partition), &area);

    if (rc != 0) return rc;
    if (area->fa_size < RTBUS_KEY_PUBLIC_SIZE) {
        flash_area_close(area);
        return -EINVAL;
    }
    rc = flash_area_read(area, 0, public_key, RTBUS_KEY_PUBLIC_SIZE);
    flash_area_close(area);
    if (rc != 0) return rc;
    if (public_key[0] != 0x04) return -EACCES;
    return 0;
}

/* Bound concurrent ECDSA workspace use by image status and upgrade callers. */
K_MUTEX_DEFINE(image_verify_lock);

int rtbus_image_verify(const struct flash_area *area, size_t signed_size)
{
    psa_hash_operation_t hash = PSA_HASH_OPERATION_INIT;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key = PSA_KEY_ID_NULL;
    uint8_t buf[128];
    uint8_t digest[32];
    uint8_t signature[RTBUS_IMAGE_P256_SIGNATURE_SIZE];
    uint8_t public_key[RTBUS_KEY_PUBLIC_SIZE];
    size_t digest_size;
    size_t offset = 0U;
    psa_status_t status;
    int rc = -EACCES;

    if (area == NULL || signed_size > area->fa_size ||
        sizeof(signature) > area->fa_size - signed_size) {
        return -EBADMSG;
    }

    k_mutex_lock(&image_verify_lock, K_FOREVER);
    rc = read_public_key(public_key);
    if (rc != 0) goto out;
    rc = -EACCES;
    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        goto out;
    }
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    status = psa_import_key(&attributes, public_key, sizeof(public_key), &key);
    if (status != PSA_SUCCESS) {
        goto out;
    }
    status = psa_hash_setup(&hash, PSA_ALG_SHA_256);
    if (status != PSA_SUCCESS) {
        goto out;
    }
    while (offset < signed_size) {
        size_t len = MIN(sizeof(buf), signed_size - offset);

        rc = flash_area_read(area, (off_t)offset, buf, len);
        if (rc != 0) {
            goto out;
        }
        rc = -EACCES;
        status = psa_hash_update(&hash, buf, len);
        if (status != PSA_SUCCESS) {
            goto out;
        }
        offset += len;
    }
    status = psa_hash_finish(&hash, digest, sizeof(digest), &digest_size);
    if (status != PSA_SUCCESS) {
        goto out;
    }
    rc = flash_area_read(area, (off_t)signed_size, signature, sizeof(signature));
    if (rc != 0) {
        goto out;
    }
    status = psa_verify_hash(key, PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                             digest, digest_size, signature, sizeof(signature));
    rc = status == PSA_SUCCESS ? 0 : -EACCES;
out:
    (void)psa_hash_abort(&hash);
    if (key != PSA_KEY_ID_NULL) {
        (void)psa_destroy_key(key);
    }
    psa_reset_key_attributes(&attributes);
    k_mutex_unlock(&image_verify_lock);
    return rc;
}
