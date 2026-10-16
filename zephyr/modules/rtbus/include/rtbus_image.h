/*
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef RTBUS_IMAGE_H_
#define RTBUS_IMAGE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTBUS_IMAGE_NAME_MAX 64U
#define RTBUS_IMAGE_VERSION_MAX 16U
#include <rtbus_image_format.h>

struct rtbus_image_native_metadata {
    uint32_t entry_offset;
    uint32_t data_load_offset;
    uint32_t data_ram_offset;
    uint32_t data_size;
    uint32_t bss_ram_offset;
    uint32_t bss_size;
};

struct rtbus_image_store_info {
    size_t image_size;
};

struct rtbus_image_status {
    bool valid;
    char name[RTBUS_IMAGE_NAME_MAX];
    char version[RTBUS_IMAGE_VERSION_MAX];
    size_t payload_size;
    uint32_t payload_crc32;
    uint32_t required_ram_size;
    uint32_t flags;
};

int rtbus_image_store_begin(const struct rtbus_image_store_info *info);
int rtbus_image_store_write(size_t offset, const uint8_t *data, size_t size);
int rtbus_image_store_finish(int status);
int rtbus_image_get_status(struct rtbus_image_status *status);
int rtbus_image_get_native_metadata(struct rtbus_image_native_metadata *metadata);
int rtbus_image_read_payload(size_t offset, uint8_t *data, size_t size,
                              size_t *read_size);

#ifdef __cplusplus
}
#endif

#endif /* RTBUS_IMAGE_H_ */
