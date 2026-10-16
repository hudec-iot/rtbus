/* SPDX-License-Identifier: MPL-2.0 */
#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(emos_entry, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_ROM(rtbus_cli_command, Z_LINK_ITERABLE_SUBALIGN)

#ifdef CONFIG_RTBUS_IMAGE_VERIFY_P256
SECTION_PROLOGUE(.rtbus_public_key, ORIGIN(RTBUS_KEY),)
{
    KEEP(*(.rtbus_public_key))
} GROUP_LINK_IN(RTBUS_KEY)

ASSERT(SIZEOF(.rtbus_public_key) == 65, "Invalid RTBus default public key")
#endif
