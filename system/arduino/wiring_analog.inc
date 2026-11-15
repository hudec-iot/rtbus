/* SPDX-License-Identifier: MPL-2.0 */
/* Included by each core's wiring_time.c; no application heap required. */
static uint32_t arduino_adc_bits = 10U;

void analogReadResolution(int bits)
{
    /* Invalid settings remain observable as -EINVAL from analogRead(). */
    arduino_adc_bits = bits >= 1 && bits <= 16 ? (uint32_t)bits : 0U;
}

int analogRead(uint32_t pin)
{
    return rtbus_analog_read(pin, arduino_adc_bits);
}
