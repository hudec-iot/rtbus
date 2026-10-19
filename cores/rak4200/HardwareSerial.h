/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__)
#define RTDUO_SERIAL_PRINTF_FORMAT __attribute__((format(printf, 2, 3)))
#else
#define RTDUO_SERIAL_PRINTF_FORMAT
#endif

class HardwareSerial {
public:
    void begin(unsigned long baud);
    void end(void);

    int available(void);
    int peek(void);
    int read(void);
    size_t readBytes(char *buffer, size_t length);
    size_t readBytes(uint8_t *buffer, size_t length);
    void flush(void);

    size_t write(uint8_t value);
    size_t write(const uint8_t *buffer, size_t size);

    size_t printf(const char *fmt, ...) RTDUO_SERIAL_PRINTF_FORMAT;
    size_t write(const char *text);
    size_t write(const char *text, size_t size);

    size_t print(const char *text);
    size_t print(char value);
    size_t print(unsigned char value, int base = 10);
    size_t print(int value, int base = 10);
    size_t print(unsigned int value, int base = 10);
    size_t print(long value, int base = 10);
    size_t print(unsigned long value, int base = 10);
    size_t print(double value, int digits = 2);
    size_t println(const char *text);
    size_t println(char value);
    size_t println(unsigned char value, int base = 10);
    size_t println(int value, int base = 10);
    size_t println(unsigned int value, int base = 10);
    size_t println(long value, int base = 10);
    size_t println(unsigned long value, int base = 10);
    size_t println(double value, int digits = 2);
    size_t println(void);
};

extern HardwareSerial Serial;

#undef RTDUO_SERIAL_PRINTF_FORMAT
