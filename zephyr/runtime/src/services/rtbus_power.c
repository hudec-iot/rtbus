/* SPDX-License-Identifier: MPL-2.0 */
#include <errno.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/printk.h>
#include <rtbus/runtime_power.h>
#include <rtbus/native_service.h>

int rtbus_runtime_power_off(void)
{
#if defined(CONFIG_POWEROFF)
    int ret = native_service_prepare_sleep();
    if (ret != 0) {
        return ret;
    }
    /* System off never returns; trace after peripheral preparation. */
    printk("rtbus: entering system off\n");
    sys_poweroff();
#else
    return -ENOTSUP;
#endif
}
