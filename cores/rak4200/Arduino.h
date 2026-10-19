/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "HardwareSerial.h"
#include "RTDuo.h"
#include "pins_arduino.h"
#include <arduino_compat.h>

typedef uint8_t byte;

#define LSBFIRST 0
#define MSBFIRST 1

#define LOW 0x0
#define HIGH 0x1
#define INPUT RUNTIME_GPIO_MODE_INPUT
#define OUTPUT RUNTIME_GPIO_MODE_OUTPUT
#define INPUT_PULLUP RUNTIME_GPIO_MODE_INPUT_PULLUP
#define INPUT_PULLDOWN RUNTIME_GPIO_MODE_INPUT_PULLDOWN

#ifdef __cplusplus
extern "C" {
#endif

void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
/* Keep IRQ-disabled sections short; do not call delay() or Serial inside. */
void noInterrupts(void);
void interrupts(void);
uint8_t shiftIn(uint32_t dataPin, uint32_t clockPin, uint8_t bitOrder);
unsigned long millis(void);
void pinMode(uint32_t pin, uint32_t mode);
void digitalWrite(uint32_t pin, uint32_t value);
int digitalRead(uint32_t pin);

#ifdef __cplusplus
}

void setup(void);
void loop(void);
#endif
