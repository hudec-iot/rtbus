/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "HardwareSerial.h"
#include "RTDuo.h"
#include "pins_arduino.h"
#include "arduino_compat.h"

typedef uint8_t byte;

#define LSBFIRST 0
#define MSBFIRST 1
#define CHANGE 2
#define FALLING 3
#define RISING 4
#define NOT_AN_INTERRUPT (-1)

typedef void (*voidFuncPtr)(void);
typedef void (*voidFuncPtrParam)(void *);
typedef uint32_t pin_size_t;
typedef int PinStatus;
typedef uint32_t PinMode;
typedef uint8_t BitOrder;

#define LOW 0x0
#define HIGH 0x1
#define INPUT RUNTIME_GPIO_MODE_INPUT
#define OUTPUT RUNTIME_GPIO_MODE_OUTPUT
#define OUTPUT_OPENDRAIN RUNTIME_GPIO_MODE_OUTPUT_OPENDRAIN
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
unsigned long micros(void);
void yield(void);
void shiftOut(uint32_t dataPin, uint32_t clockPin, uint8_t bitOrder, uint8_t value);
int digitalPinToInterrupt(uint32_t pin);
void attachInterrupt(uint32_t interrupt, voidFuncPtr callback, int mode);
void attachInterruptParam(uint32_t interrupt, voidFuncPtrParam callback, int mode, void *argument);
void detachInterrupt(uint32_t interrupt);
/* Last failure from void GPIO/interrupt APIs, as a negative RTBus errno. */
int arduinoLastError(void);
void pinMode(uint32_t pin, uint32_t mode);
void digitalWrite(uint32_t pin, uint32_t value);
int digitalRead(uint32_t pin);

#ifdef __cplusplus
}

void setup(void);
void loop(void);
unsigned long pulseIn(uint32_t pin, uint8_t state, unsigned long timeout = 1000000UL);
unsigned long pulseInLong(uint32_t pin, uint8_t state, unsigned long timeout = 1000000UL);
#endif
