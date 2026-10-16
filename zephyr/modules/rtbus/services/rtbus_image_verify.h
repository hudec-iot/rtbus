/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RTBUS_IMAGE_VERIFY_H_
#define RTBUS_IMAGE_VERIFY_H_

#include <runtime_macro.h>
#include <stddef.h>
struct flash_area;

/* Verify ECDSA P-256/SHA-256 over the on-flash header + payload. */
int rtbus_image_verify(const struct flash_area *area, size_t signed_size);

#endif
