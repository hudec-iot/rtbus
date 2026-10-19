/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stddef.h>
#include <stdint.h>
class __FlashStringHelper;
#ifndef F
#define F(text) (reinterpret_cast<const __FlashStringHelper *>(text))
#endif
class String {
    char *buffer_ = nullptr;
    unsigned int length_ = 0;
    unsigned int capacity_ = 0;
    void invalidate();
public:
    String(const char *text = "");
    String(const char *text, unsigned int length);
    String(const uint8_t *text, unsigned int length) : String((const char *)text, length) {}
    String(const __FlashStringHelper *text) : String((const char *)text) {}
    String(const String &other);
    String(String &&other) noexcept;
    explicit String(char value);
    explicit String(unsigned char value, unsigned char base = 10);
    explicit String(int value, unsigned char base = 10);
    explicit String(unsigned int value, unsigned char base = 10);
    explicit String(long value, unsigned char base = 10);
    explicit String(unsigned long value, unsigned char base = 10);
    explicit String(float value, unsigned char digits = 2);
    explicit String(double value, unsigned char digits = 2);
    ~String();
    bool reserve(unsigned int size);
    unsigned int length() const { return length_; }
    bool isEmpty() const { return length_ == 0; }
    operator bool() const { return buffer_ != nullptr; }
    const char *c_str() const { return buffer_ ? buffer_ : ""; }
    char *begin() { return buffer_; }
    const char *begin() const { return c_str(); }
    char *end() { return buffer_ ? buffer_ + length_ : nullptr; }
    const char *end() const { return c_str() + length_; }
    String &operator=(const String &other);
    String &operator=(String &&other) noexcept;
    String &operator=(const char *text);
    String &operator=(const __FlashStringHelper *text) { return operator=((const char *)text); }
    bool concat(const char *text, unsigned int length);
    bool concat(const char *text);
    bool concat(const uint8_t *text, unsigned int length) { return concat((const char *)text, length); }
    bool concat(const String &text) { return text ? concat(text.c_str(), text.length()) : false; }
    bool concat(const __FlashStringHelper *text) { return concat((const char *)text); }
    bool concat(char value) { return concat(&value, 1); }
    bool concat(unsigned char value);
    bool concat(int value);
    bool concat(unsigned int value);
    bool concat(long value);
    bool concat(unsigned long value);
    bool concat(float value);
    bool concat(double value);
    template<class T> String &operator+=(const T &value) { concat(value); return *this; }
    int compareTo(const String &other) const { return compareTo(other.c_str()); }
    int compareTo(const char *other) const;
    bool equals(const String &other) const;
    bool equals(const char *other) const;
    bool equalsIgnoreCase(const String &other) const;
    bool startsWith(const String &prefix, unsigned int offset = 0) const;
    bool endsWith(const String &suffix) const;
    char charAt(unsigned int index) const { return index < length_ ? buffer_[index] : 0; }
    void setCharAt(unsigned int index, char value) { if (index < length_) { buffer_[index] = value; } }
    char operator[](unsigned int index) const { return charAt(index); }
    char &operator[](unsigned int index);
    void getBytes(unsigned char *output, unsigned int size, unsigned int index = 0) const;
    void toCharArray(char *output, unsigned int size, unsigned int index = 0) const { getBytes((unsigned char *)output, size, index); }
    int indexOf(char value, unsigned int from = 0) const;
    int indexOf(const String &value, unsigned int from = 0) const;
    int lastIndexOf(char value) const;
    int lastIndexOf(char value, unsigned int from) const;
    int lastIndexOf(const String &value) const;
    int lastIndexOf(const String &value, unsigned int from) const;
    String substring(unsigned int from) const { return substring(from, length_); }
    String substring(unsigned int from, unsigned int to) const;
    void replace(char from, char to);
    void replace(const String &from, const String &to);
    void remove(unsigned int from) { remove(from, length_); }
    void remove(unsigned int from, unsigned int count);
    void toLowerCase();
    void toUpperCase();
    void trim();
    long toInt() const;
    float toFloat() const { return (float)toDouble(); }
    double toDouble() const;
    friend bool operator==(const String &a, const String &b) { return a.equals(b); }
    friend bool operator==(const String &a, const char *b) { return a.equals(b); }
    friend bool operator==(const char *a, const String &b) { return b.equals(a); }
    friend bool operator!=(const String &a, const String &b) { return !a.equals(b); }
    friend bool operator!=(const String &a, const char *b) { return !a.equals(b); }
    friend bool operator!=(const char *a, const String &b) { return !b.equals(a); }
    friend bool operator<(const String &a, const String &b) { return a.compareTo(b) < 0; }
    friend bool operator>(const String &a, const String &b) { return a.compareTo(b) > 0; }
    friend bool operator<=(const String &a, const String &b) { return a.compareTo(b) <= 0; }
    friend bool operator>=(const String &a, const String &b) { return a.compareTo(b) >= 0; }
};
using StringSumHelper = String;
template<class T> inline String operator+(String left, const T &right) { left.concat(right); return left; }
inline String operator+(const char *left, const String &right) { String result(left); result.concat(right); return result; }
inline String operator+(char left, const String &right) { String result(left); result.concat(right); return result; }
