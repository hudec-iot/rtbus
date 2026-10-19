/* SPDX-License-Identifier: MPL-2.0 */
#include "Arduino.h"

#include <runtime_api.h>

static int arduino_last_error;

extern "C" int arduinoLastError(void) { return arduino_last_error; }

extern "C" void pinMode(uint32_t pin, uint32_t mode) {
    arduino_last_error = rtbus_gpio_configure(pin, mode);
}

extern "C" void digitalWrite(uint32_t pin, uint32_t value) {
    arduino_last_error = rtbus_gpio_write(pin, value);
}

extern "C" int digitalRead(uint32_t pin) {
    int value = rtbus_gpio_read(pin);
    arduino_last_error = value < 0 ? value : 0;
    return value > 0 ? 1 : 0;
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

extern "C" void shiftOut(uint32_t dataPin, uint32_t clockPin, uint8_t bitOrder, uint8_t value)
{
    for (uint8_t bit = 0; bit < 8; ++bit) {
        digitalWrite(dataPin, (value >> (bitOrder == LSBFIRST ? bit : 7 - bit)) & 1U);
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(clockPin, LOW);
        delayMicroseconds(1);
    }
}

extern "C" int digitalPinToInterrupt(uint32_t pin) { return (int)pin; }
static uint32_t interruptMode(int mode)
{
    switch (mode) {
    case LOW: return RTBUS_GPIO_IRQ_LOW;
    case HIGH: return RTBUS_GPIO_IRQ_HIGH;
    case CHANGE: return RTBUS_GPIO_IRQ_CHANGE;
    case FALLING: return RTBUS_GPIO_IRQ_FALLING;
    case RISING: return RTBUS_GPIO_IRQ_RISING;
    default: return UINT32_MAX;
    }
}
static void interruptThunk(void *argument)
{
    voidFuncPtr callback = reinterpret_cast<voidFuncPtr>((uintptr_t)argument);
    callback();
}
extern "C" void attachInterrupt(uint32_t interrupt, voidFuncPtr callback, int mode)
{
    arduino_last_error = rtbus_gpio_interrupt(interrupt, interruptMode(mode),
        callback ? interruptThunk : nullptr, reinterpret_cast<void *>((uintptr_t)callback));
}
extern "C" void attachInterruptParam(uint32_t interrupt, voidFuncPtrParam callback, int mode, void *argument)
{
    arduino_last_error = rtbus_gpio_interrupt(interrupt, interruptMode(mode), callback, argument);
}
extern "C" void detachInterrupt(uint32_t interrupt)
{
    arduino_last_error = rtbus_gpio_interrupt(interrupt, RTBUS_GPIO_IRQ_DETACH, nullptr, nullptr);
}

unsigned long pulseInLong(uint32_t pin, uint8_t state, unsigned long timeout)
{
    const int level = state != LOW;
    unsigned long started = micros();
    // Finish any existing pulse, wait for a new pulse, then measure it.
    for (unsigned int phase = 0; phase < 3; ++phase) {
        unsigned long pulseStart = micros();
        int waitingLevel = phase == 1 ? !level : level;
        while (true) {
            int value = rtbus_gpio_read(pin);
            if (value < 0) { arduino_last_error = value; return 0; }
            unsigned long now = micros();
            if ((unsigned long)(now - started) >= timeout) { return 0; }
            if (value != waitingLevel) {
                if (phase == 2) { arduino_last_error = 0; return now - pulseStart; }
                break;
            }
        }
    }
    return 0;
}
unsigned long pulseIn(uint32_t pin, uint8_t state, unsigned long timeout)
{
    return pulseInLong(pin, state, timeout);
}
