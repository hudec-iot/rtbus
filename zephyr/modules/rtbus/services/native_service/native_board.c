/*
 * SPDX-License-Identifier: MPL-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include "native_board.h"

#if DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1262) && defined(CONFIG_SPI) && !defined(CONFIG_LORA)
#define NATIVE_SLEEP_RADIO DT_COMPAT_GET_ANY_STATUS_OKAY(semtech_sx1262)
static const struct spi_dt_spec sleep_radio_spi = SPI_DT_SPEC_GET(
    NATIVE_SLEEP_RADIO, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB);
static const struct gpio_dt_spec sleep_radio_reset =
    GPIO_DT_SPEC_GET(NATIVE_SLEEP_RADIO, reset_gpios);
static const struct gpio_dt_spec sleep_radio_busy =
    GPIO_DT_SPEC_GET(NATIVE_SLEEP_RADIO, busy_gpios);
#endif

int native_board_prepare_sleep(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1262)
#if defined(CONFIG_LORA) || !defined(CONFIG_SPI)
    /* Raw SPI shutdown must not race an active LoRa driver. A driver-owned
     * shutdown path is required before enabling this combination.
     */
    return -ENOTSUP;
#else
    if (!spi_is_ready_dt(&sleep_radio_spi) ||
        !gpio_is_ready_dt(&sleep_radio_reset) ||
        !gpio_is_ready_dt(&sleep_radio_busy)) {
        return -ENODEV;
    }

    /* The radio is deferred/uninitialized when CONFIG_LORA is off. Reset
     * gives a known standby state even after an MCU-only reset or if the
     * radio was already asleep. This deliberately discards radio settings.
     */
    int ret = gpio_pin_configure_dt(&sleep_radio_busy, GPIO_INPUT);
    if (ret != 0) { return ret; }
    ret = gpio_pin_configure_dt(&sleep_radio_reset, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) { return ret; }
    k_sleep(K_MSEC(1));
    ret = gpio_pin_set_dt(&sleep_radio_reset, 0);
    if (ret != 0) { return ret; }
    k_sleep(K_MSEC(10));

    int64_t deadline = k_uptime_get() + 100;
    while (true) {
        ret = gpio_pin_get_dt(&sleep_radio_busy);
        if (ret < 0) { return ret; }
        if (ret == 0) { break; }
        if (k_uptime_get() >= deadline) { return -ETIMEDOUT; }
        k_sleep(K_MSEC(1));
    }

    /* SX1262 SetSleep: cold start, RTC wake disabled. Sleep stops the radio
     * oscillator/TCXO supply. Keep NSS high afterwards: lowering it wakes
     * the radio. No post-command BUSY polling (BUSY is high in sleep).
     */
    const uint8_t command[] = { 0x84, 0x00 };
    const struct spi_buf buffer = {
        .buf = (void *)command,
        .len = sizeof(command),
    };
    const struct spi_buf_set buffers = { .buffers = &buffer, .count = 1 };
    ret = spi_write_dt(&sleep_radio_spi, &buffers);
    if (ret != 0) { return ret; }
    k_sleep(K_MSEC(1));
#endif
#endif
#if defined(CONFIG_BOARD_RAK4631)
    /* RAK4631 RF_IO is held high by the boot GPIO hog. Drive P1.05 low
     * only after the radio has entered sleep, before MCU system off.
     * Reset reapplies the board's output-high boot configuration.
     */
    const struct gpio_dt_spec rf_io = {
        .port = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
        .pin = 5,
        .dt_flags = GPIO_ACTIVE_HIGH,
    };
    if (!gpio_is_ready_dt(&rf_io)) {
        return -ENODEV;
    }
    int rf_ret = gpio_pin_configure_dt(&rf_io, GPIO_OUTPUT_INACTIVE);
    if (rf_ret != 0) {
        return rf_ret;
    }
#endif
    return 0;
}

#define RTBUS_NATIVE_GPIO_NODE DT_NODELABEL(rtbus_native_gpio)

#define NATIVE_BOARD_GPIO_PIN_ENTRY(node_id) \
    {                                        \
        .app_pin = DT_PROP(node_id, rtbus_pin), \
        .gpio = GPIO_DT_SPEC_GET(node_id, gpios), \
    }

#if DT_NODE_EXISTS(RTBUS_NATIVE_GPIO_NODE)
#if DT_CHILD_NUM(RTBUS_NATIVE_GPIO_NODE) > 0
static const struct native_board_gpio_pin native_gpio_pins[] = {
    DT_FOREACH_CHILD_SEP(RTBUS_NATIVE_GPIO_NODE,
                         NATIVE_BOARD_GPIO_PIN_ENTRY,
                         (,))
};
#endif
#endif

const struct native_board_gpio_pin *native_board_gpio_pins(size_t *count)
{
#if DT_NODE_EXISTS(RTBUS_NATIVE_GPIO_NODE)
#if DT_CHILD_NUM(RTBUS_NATIVE_GPIO_NODE) > 0
    *count = ARRAY_SIZE(native_gpio_pins);
    return native_gpio_pins;
#else
    *count = 0U;
    return NULL;
#endif
#else
    *count = 0U;
    return NULL;
#endif
}
