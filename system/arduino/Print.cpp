/* SPDX-License-Identifier: MPL-2.0 */
#include "Print.h"
#include "WString.h"

static size_t arduino_text_length(const char *text)
{
    size_t count = 0;
    if (text) { while (text[count]) { ++count; } }
    return count;
}

size_t Print::write(const uint8_t *buffer, size_t size)
{
    if (!buffer) { return 0; }
    size_t count = 0;
    while (count < size && write(buffer[count]) == 1) { ++count; }
    return count;
}

// Common formatting for Serial and user-defined Print sinks.
static size_t serial_print_unsigned(Print *serial, unsigned long value, int base)
{
    if (base == 0) {
        return serial->write((uint8_t)value);
    }
    if (base < 2 || base > 36) {
        base = 10;
    }
    char buffer[sizeof(value) * 8];
    size_t pos = sizeof(buffer);
    do {
        unsigned int digit = value % (unsigned int)base;
        buffer[--pos] = (char)(digit < 10 ? '0' + digit : 'A' + digit - 10);
        value /= (unsigned int)base;
    } while (value != 0);
    return serial->write((const uint8_t *)buffer + pos, sizeof(buffer) - pos);
}

size_t Print::write(const char *text)
{
    return print(text);
}

size_t Print::write(const char *text, size_t size)
{
    return write(reinterpret_cast<const uint8_t *>(text), size);
}

size_t Print::print(const char *text)
{
    return write(reinterpret_cast<const uint8_t *>(text), arduino_text_length(text));
}

size_t Print::print(char value)
{
    return write((uint8_t)value);
}

size_t Print::print(unsigned char value, int base)
{
    return print((unsigned long)value, base);
}

size_t Print::print(int value, int base)
{
    return print((long)value, base);
}

size_t Print::print(unsigned int value, int base)
{
    return print((unsigned long)value, base);
}

size_t Print::print(long value, int base)
{
    unsigned long magnitude = (unsigned long)value;
    size_t written = 0;
    if (base == 10 && value < 0) {
        written = print('-');
        magnitude = 0UL - magnitude;
    }
    return written + serial_print_unsigned(this, magnitude, base);
}

size_t Print::print(unsigned long value, int base)
{
    return serial_print_unsigned(this, value, base);
}

size_t Print::print(double value, int digits)
{
    // Fixed-decimal Arduino formatting without application libc/libm or heap.
    if (__builtin_isnan(value)) {
        return print("nan");
    }
    if (__builtin_isinf(value)) {
        return print("inf");
    }
    if (value > 4294967040.0 || value < -4294967040.0) {
        return print("ovf");
    }
    if (digits < 0) {
        digits = 2;
    }
    size_t written = 0;
    if (value < 0) {
        written += print('-');
        value = -value;
    }
    double halfUnit = 0.5;
    for (int place = 0; place < digits; ++place) {
        halfUnit *= 0.1;
    }
    value += halfUnit;
    unsigned long whole = (unsigned long)value;
    written += print(whole);
    double fraction = value - whole;
    if (digits > 0) {
        written += print('.');
    }
    for (int place = 0; place < digits; ++place) {
        fraction *= 10;
        unsigned int digit = (unsigned int)fraction;
        written += print((char)('0' + digit));
        fraction -= digit;
    }
    return written;
}

size_t Print::println(const char *text)
{
    size_t written = print(text);
    return written + println();
}

size_t Print::println(char value)
{
    size_t written = print(value);
    return written + println();
}

size_t Print::println(double value, int digits)
{
    size_t written = print(value, digits);
    return written + println();
}

size_t Print::println(unsigned char value, int base)
{
    size_t written = print(value, base);
    return written + println();
}

size_t Print::println(int value, int base)
{
    size_t written = print(value, base);
    return written + println();
}

size_t Print::println(unsigned int value, int base)
{
    size_t written = print(value, base);
    return written + println();
}

size_t Print::println(long value, int base)
{
    size_t written = print(value, base);
    return written + println();
}

size_t Print::println(unsigned long value, int base)
{
    size_t written = print(value, base);
    return written + println();
}

size_t Print::println(void)
{
    static const uint8_t newline[] = {'\r', '\n'};
    return write(newline, sizeof(newline));
}

size_t Print::print(const Printable &value) { return value.printTo(*this); }
size_t Print::print(const String &value) { return write((const uint8_t *)value.c_str(), value.length()); }
size_t Print::print(const __FlashStringHelper *value) { return print(reinterpret_cast<const char *>(value)); }
size_t Print::println(const Printable &value) { size_t n = print(value); return n + println(); }
size_t Print::println(const String &value) { size_t n = print(value); return n + println(); }
size_t Print::println(const __FlashStringHelper *value) { size_t n = print(value); return n + println(); }

size_t Print::print(unsigned long long value, int base)
{
    if (base == 0) { return write((uint8_t)value); }
    if (base < 2 || base > 36) { base = 10; }
    char buffer[64];
    size_t pos = sizeof(buffer);
    do {
        unsigned int digit = value % (unsigned int)base;
        buffer[--pos] = (char)(digit < 10 ? '0' + digit : 'A' + digit - 10);
        value /= (unsigned int)base;
    } while (value);
    return write((const uint8_t *)buffer + pos, sizeof(buffer) - pos);
}
size_t Print::print(long long value, int base)
{
    unsigned long long magnitude = (unsigned long long)value;
    size_t n = 0;
    if (value < 0 && base == 10) { n = print('-'); magnitude = 0ULL - magnitude; }
    return n + print(magnitude, base);
}
size_t Print::println(long long value, int base) { size_t n = print(value, base); return n + println(); }
size_t Print::println(unsigned long long value, int base) { size_t n = print(value, base); return n + println(); }
