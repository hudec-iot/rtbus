/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "Printable.h"
class String;
class __FlashStringHelper;
#ifndef F
#define F(text) (reinterpret_cast<const __FlashStringHelper *>(text))
#endif
class Print {
    int writeError_ = 0;
protected:
    void setWriteError(int error = 1) { writeError_ = error; }
public:
    constexpr Print() = default;
    int getWriteError() const { return writeError_; }
    void clearWriteError() { writeError_ = 0; }
    virtual int availableForWrite() { return 0; }
    virtual void flush() {}
    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size);
    size_t write(int value) { return write((uint8_t)value); }
    size_t write(unsigned int value) { return write((uint8_t)value); }
    size_t write(long value) { return write((uint8_t)value); }
    size_t write(unsigned long value) { return write((uint8_t)value); }

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
    size_t print(long long value, int base = 10);
    size_t print(unsigned long long value, int base = 10);
    size_t print(const Printable &value);
    size_t print(const String &value);
    size_t print(const __FlashStringHelper *value);
    size_t println(long long value, int base = 10);
    size_t println(unsigned long long value, int base = 10);
    size_t println(const Printable &value);
    size_t println(const String &value);
    size_t println(const __FlashStringHelper *value);
};
