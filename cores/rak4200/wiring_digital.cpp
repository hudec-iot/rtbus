/* SPDX-License-Identifier: MPL-2.0 */
#include <stdint.h>

#include <runtime_api.h>

extern "C" void pinMode(uint32_t pin, uint32_t mode) {
    (void)rtbus_gpio_configure(pin, mode);
}

extern "C" void digitalWrite(uint32_t pin, uint32_t value) {
    (void)rtbus_gpio_write(pin, value);
}

extern "C" int digitalRead(uint32_t pin) {
    return rtbus_gpio_read(pin) > 0 ? 1 : 0;
}

/* One microsecond per half-cycle also supports fast CPUs behind the ABI.
 * Callers needing uninterrupted transfers must bracket the whole transaction
 * with noInterrupts()/interrupts(), including any trailing protocol clocks.
 */
extern "C" uint8_t shiftIn(uint32_t dataPin, uint32_t clockPin, uint8_t bitOrder) {
    uint8_t value = 0;

    for (uint8_t bit = 0; bit < 8; ++bit) {
        digitalWrite(clockPin, 1);
        rtbus_delay_us(1);
        int sample = digitalRead(dataPin);
        digitalWrite(clockPin, 0);
        rtbus_delay_us(1);
        value |= (uint8_t)(sample << (bitOrder == 0 ? bit : 7 - bit));
    }

    return value;
}
