/* SPDX-License-Identifier: MPL-2.0 */
#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "native_board.h"

#define NATIVE_ADC_NODE DT_NODELABEL(rtbus_native_adc)

#if defined(CONFIG_ADC) && DT_NODE_EXISTS(NATIVE_ADC_NODE)
struct native_adc_pin {
    uint32_t pin;
    struct adc_dt_spec adc;
    struct gpio_dt_spec gpio;
};
#define NATIVE_ADC_ENTRY(node) \
    { DT_PROP(node, rtbus_pin), ADC_DT_SPEC_GET(node), GPIO_DT_SPEC_GET(node, gpios) }
static const struct native_adc_pin native_adc_pins[] = {
    DT_FOREACH_CHILD_SEP(NATIVE_ADC_NODE, NATIVE_ADC_ENTRY, (,))
};
K_MUTEX_DEFINE(native_adc_lock);
#endif

int native_board_analog_read(uint32_t pin, uint32_t bits)
{
    if (bits < 1U || bits > 16U) {
        return -EINVAL;
    }
#if defined(CONFIG_ADC) && DT_NODE_EXISTS(NATIVE_ADC_NODE)
    for (size_t i = 0; i < ARRAY_SIZE(native_adc_pins); ++i) {
        const struct native_adc_pin *entry = &native_adc_pins[i];
        if (entry->pin != pin) { continue; }
        if (!adc_is_ready_dt(&entry->adc) || !gpio_is_ready_dt(&entry->gpio)) {
            return -ENODEV;
        }
        if (entry->adc.channel_cfg.differential ||
            entry->adc.resolution == 0U || entry->adc.resolution > 15U) {
            return -ENOTSUP;
        }
        int ret = k_mutex_lock(&native_adc_lock, K_FOREVER);
        if (ret != 0) { return ret; }
        int16_t sample = 0;
        struct adc_sequence sequence = {
            .buffer = &sample,
            .buffer_size = sizeof(sample),
        };
        /* analogRead takes ownership of the pin; a prior digital output
         * or pull resistor must not drive the signal during conversion.
         */
        ret = gpio_pin_configure_dt(&entry->gpio, GPIO_DISCONNECTED);
        if (ret == 0) { ret = adc_channel_setup_dt(&entry->adc); }
        if (ret == 0) { ret = adc_sequence_init_dt(&entry->adc, &sequence); }
        if (ret == 0) { ret = adc_read_dt(&entry->adc, &sequence); }
        k_mutex_unlock(&native_adc_lock);
        if (ret != 0) { return ret; }
        uint32_t value = sample < 0 ? 0U : (uint32_t)sample;
        uint32_t maximum = (1U << entry->adc.resolution) - 1U;
        if (value > maximum) { value = maximum; }
        return (int)(bits > entry->adc.resolution ?
            value << (bits - entry->adc.resolution) :
            value >> (entry->adc.resolution - bits));
    }
    return -EINVAL;
#else
    ARG_UNUSED(pin);
    return -ENOTSUP;
#endif
}
