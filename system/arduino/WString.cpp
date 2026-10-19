/* SPDX-License-Identifier: MPL-2.0 */
#include "WString.h"
#include "Print.h"
#include <limits.h>

// Application-owned storage, linked only when String allocation is used.
// Configure through compiler flags, not by borrowing the runtime's heap.
#ifndef RTDUO_STRING_POOL_SIZE
#define RTDUO_STRING_POOL_SIZE 1024
#endif
static_assert(RTDUO_STRING_POOL_SIZE >= 32 && RTDUO_STRING_POOL_SIZE <= 65520,
              "RTDUO_STRING_POOL_SIZE must be between 32 and 65520");
static const unsigned int stringChunk = 16;
static char stringPool[RTDUO_STRING_POOL_SIZE];
static uint16_t stringAllocations[RTDUO_STRING_POOL_SIZE / stringChunk];
static char *stringAllocate(unsigned int bytes)
{
    if (!bytes || bytes > sizeof(stringPool)) { return nullptr; }
    unsigned int chunks = (bytes + stringChunk - 1) / stringChunk;
    unsigned int run = 0;
    for (unsigned int i = 0; i < sizeof(stringAllocations) / sizeof(stringAllocations[0]); ++i) {
        run = stringAllocations[i] == 0 ? run + 1 : 0;
        if (run == chunks) {
            unsigned int start = i + 1 - chunks;
            stringAllocations[start] = (uint16_t)chunks;
            for (unsigned int j = start + 1; j <= i; ++j) { stringAllocations[j] = UINT16_MAX; }
            return stringPool + start * stringChunk;
        }
    }
    return nullptr;
}
static void stringRelease(char *buffer)
{
    if (!buffer) { return; }
    unsigned int start = (unsigned int)(buffer - stringPool) / stringChunk;
    unsigned int count = stringAllocations[start];
    for (unsigned int i = 0; i < count; ++i) { stringAllocations[start + i] = 0; }
}
static unsigned int stringLength(const char *text)
{
    unsigned int n = 0; if (text) { while (text[n]) { ++n; } } return n;
}
static char lowerAscii(char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; }
static bool spaceAscii(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
void String::invalidate() { stringRelease(buffer_); buffer_ = nullptr; length_ = capacity_ = 0; }
String::~String() { stringRelease(buffer_); }
bool String::reserve(unsigned int size)
{
    if (buffer_ && size <= capacity_) { return true; }
    if (size == UINT_MAX) { return false; }
    char *replacement = stringAllocate(size + 1);
    if (!replacement) { return false; }
    for (unsigned int i = 0; i < length_; ++i) { replacement[i] = buffer_[i]; }
    replacement[length_] = 0;
    stringRelease(buffer_);
    buffer_ = replacement;
    capacity_ = ((size + 1 + stringChunk - 1) / stringChunk) * stringChunk - 1;
    return true;
}
String::String(const char *text) : String(text, stringLength(text)) {}
String::String(const char *text, unsigned int length)
{
    if (text && reserve(length)) {
        for (unsigned int i = 0; i < length; ++i) { buffer_[i] = text[i]; }
        buffer_[length] = 0; length_ = length;
    }
}
String::String(const String &other) : String(other ? other.c_str() : nullptr, other.length_) {}
String::String(String &&other) noexcept
    : buffer_(other.buffer_), length_(other.length_), capacity_(other.capacity_)
{ other.buffer_ = nullptr; other.length_ = other.capacity_ = 0; }
String &String::operator=(String &&other) noexcept
{
    if (this != &other) {
        invalidate(); buffer_ = other.buffer_; length_ = other.length_; capacity_ = other.capacity_;
        other.buffer_ = nullptr; other.length_ = other.capacity_ = 0;
    }
    return *this;
}
String &String::operator=(const String &other)
{
    if (this != &other) { operator=(other ? other.c_str() : nullptr); }
    return *this;
}
String &String::operator=(const char *text)
{
    if (!text) { invalidate(); return *this; }
    unsigned int size = stringLength(text);
    // Interior pointers cannot require growth; copy forward for alias safety.
    if (!reserve(size)) { invalidate(); return *this; }
    for (unsigned int i = 0; i < size; ++i) { buffer_[i] = text[i]; }
    length_ = size; buffer_[size] = 0;
    return *this;
}
bool String::concat(const char *text) { return text && concat(text, stringLength(text)); }
bool String::concat(const char *text, unsigned int size)
{
    if (!text || size > UINT_MAX - length_) { return false; }
    // Self-concatenation may reallocate the backing buffer.
    uintptr_t address = (uintptr_t)text, start = (uintptr_t)buffer_;
    bool alias = buffer_ && address >= start && address <= start + length_;
    unsigned int offset = alias ? (unsigned int)(address - start) : 0;
    if (alias && size > length_ - offset) { return false; }
    if (!reserve(length_ + size)) { return false; }
    if (alias) { text = buffer_ + offset; }
    for (unsigned int i = 0; i < size; ++i) { buffer_[length_ + i] = text[i]; }
    length_ += size; buffer_[length_] = 0;
    return true;
}
class StringOutput : public Print {
    String &target_;
    bool failed_ = false;
public:
    explicit StringOutput(String &target) : target_(target) {}
    size_t write(uint8_t c) override {
        if (failed_ || !target_.concat((char)c)) { failed_ = true; return 0; }
        return 1;
    }
    bool failed() const { return failed_; }
};
String::String(char value) { reserve(1); concat(value); }
String::String(unsigned char value, unsigned char base) { reserve(32); StringOutput sink(*this); sink.print(value, base); if (sink.failed()) { invalidate(); } }
String::String(int value, unsigned char base) { reserve(32); StringOutput sink(*this); sink.print(value, base); if (sink.failed()) { invalidate(); } }
String::String(unsigned int value, unsigned char base) { reserve(32); StringOutput sink(*this); sink.print(value, base); if (sink.failed()) { invalidate(); } }
String::String(long value, unsigned char base) { reserve(32); StringOutput sink(*this); sink.print(value, base); if (sink.failed()) { invalidate(); } }
String::String(unsigned long value, unsigned char base) { reserve(32); StringOutput sink(*this); sink.print(value, base); if (sink.failed()) { invalidate(); } }
String::String(float value, unsigned char digits) { reserve(16U + digits); StringOutput sink(*this); sink.print((double)value, digits); if (sink.failed()) { invalidate(); } }
String::String(double value, unsigned char digits) { reserve(16U + digits); StringOutput sink(*this); sink.print((double)value, digits); if (sink.failed()) { invalidate(); } }
bool String::concat(unsigned char value) { String text(value); return concat(text); }
bool String::concat(int value) { String text(value); return concat(text); }
bool String::concat(unsigned int value) { String text(value); return concat(text); }
bool String::concat(long value) { String text(value); return concat(text); }
bool String::concat(unsigned long value) { String text(value); return concat(text); }
bool String::concat(float value) { String text(value); return concat(text); }
bool String::concat(double value) { String text(value); return concat(text); }
int String::compareTo(const char *other) const
{
    if (!other) { other = ""; }
    const unsigned char *a = (const unsigned char *)c_str(), *b = (const unsigned char *)other;
    while (*a && *a == *b) { ++a; ++b; } return *a - *b;
}
bool String::equals(const String &other) const
{
    if (length_ != other.length_) { return false; }
    for (unsigned int i = 0; i < length_; ++i) { if (buffer_[i] != other.buffer_[i]) { return false; } }
    return true;
}
bool String::equals(const char *other) const { return length_ == stringLength(other) && compareTo(other) == 0; }
bool String::equalsIgnoreCase(const String &other) const
{
    if (length_ != other.length_) { return false; }
    for (unsigned int i = 0; i < length_; ++i) { if (lowerAscii(buffer_[i]) != lowerAscii(other.buffer_[i])) { return false; } }
    return true;
}
bool String::startsWith(const String &prefix, unsigned int offset) const
{
    if (offset > length_ || prefix.length_ > length_ - offset) { return false; }
    for (unsigned int i = 0; i < prefix.length_; ++i) { if (buffer_[offset + i] != prefix.buffer_[i]) { return false; } }
    return true;
}
bool String::endsWith(const String &suffix) const { return suffix.length_ <= length_ && startsWith(suffix, length_ - suffix.length_); }
char &String::operator[](unsigned int index)
{
    static char invalid;
    if (index < length_) { return buffer_[index]; }
    invalid = 0; return invalid;
}
void String::getBytes(unsigned char *output, unsigned int size, unsigned int index) const
{
    if (!output || !size) { return; }
    unsigned int n = 0;
    while (n < size - 1 && index < length_) { output[n++] = buffer_[index++]; }
    output[n] = 0;
}
int String::indexOf(char value, unsigned int from) const
{ for (unsigned int i = from; i < length_; ++i) { if (buffer_[i] == value) { return (int)i; } } return -1; }
int String::indexOf(const String &value, unsigned int from) const
{
    if (value.length_ > length_ || from > length_) { return -1; }
    for (unsigned int i = from; i <= length_ - value.length_; ++i) { if (startsWith(value, i)) { return (int)i; } }
    return -1;
}
int String::lastIndexOf(char value) const { return lastIndexOf(value, length_); }
int String::lastIndexOf(char value, unsigned int from) const
{
    if (!length_) { return -1; }
    unsigned int i = from < length_ ? from : length_ - 1;
    do { if (buffer_[i] == value) { return (int)i; } } while (i--);
    return -1;
}
int String::lastIndexOf(const String &value) const { return lastIndexOf(value, length_); }
int String::lastIndexOf(const String &value, unsigned int from) const
{
    if (value.length_ > length_) { return -1; }
    unsigned int last = length_ - value.length_, i = from < last ? from : last;
    do { if (startsWith(value, i)) { return (int)i; } } while (i--);
    return -1;
}
String String::substring(unsigned int from, unsigned int to) const
{
    if (from > to) { unsigned int swap = from; from = to; to = swap; }
    if (from > length_) { from = length_; }
    if (to > length_) { to = length_; }
    return String(c_str() + from, to - from);
}
void String::replace(char from, char to) { for (unsigned int i = 0; i < length_; ++i) { if (buffer_[i] == from) { buffer_[i] = to; } } }
void String::replace(const String &from, const String &to)
{
    if (!from.length_) { return; }
    String result;
    if (!result.reserve(length_)) { return; }
    for (unsigned int i = 0; i < length_;) {
        if (startsWith(from, i)) { if (!result.concat(to)) { return; } i += from.length_; }
        else { if (!result.concat(buffer_[i++])) { return; } }
    }
    *this = static_cast<String &&>(result);
}
void String::remove(unsigned int from, unsigned int count)
{
    if (from >= length_) { return; }
    if (count > length_ - from) { count = length_ - from; }
    for (unsigned int i = from; i + count <= length_; ++i) { buffer_[i] = buffer_[i + count]; }
    length_ -= count;
}
void String::toLowerCase() { for (unsigned int i = 0; i < length_; ++i) { buffer_[i] = lowerAscii(buffer_[i]); } }
void String::toUpperCase() { for (unsigned int i = 0; i < length_; ++i) { char c = buffer_[i]; if (c >= 'a' && c <= 'z') { buffer_[i] = c - ('a' - 'A'); } } }
void String::trim()
{
    unsigned int start = 0, end = length_;
    while (start < end && spaceAscii(buffer_[start])) { ++start; }
    while (end > start && spaceAscii(buffer_[end - 1])) { --end; }
    remove(end); remove(0, start);
}
long String::toInt() const
{
    const char *p = c_str(); while (spaceAscii(*p)) { ++p; }
    bool negative = *p == '-'; if (*p == '+' || *p == '-') { ++p; }
    unsigned long value = 0;
    while (*p >= '0' && *p <= '9') { value = value * 10 + *p++ - '0'; }
    return (long)(negative ? 0UL - value : value);
}
double String::toDouble() const
{
    const char *p = c_str(); while (spaceAscii(*p)) { ++p; }
    bool negative = *p == '-'; if (*p == '+' || *p == '-') { ++p; }
    double value = 0, factor = 1;
    bool any = false;
    while (*p >= '0' && *p <= '9') { any = true; value = value * 10 + *p++ - '0'; }
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') { any = true; factor *= 0.1; value += (*p++ - '0') * factor; }
    }
    if (any && (*p == 'e' || *p == 'E')) {
        ++p; bool inverse = *p == '-'; if (*p == '+' || *p == '-') { ++p; }
        unsigned int exponent = 0;
        while (*p >= '0' && *p <= '9') { if (exponent < 4096) { exponent = exponent * 10 + *p - '0'; } ++p; }
        // Bound work for malformed or out-of-range input; double already saturates.
        if (exponent > 4096) { exponent = 4096; }
        while (exponent--) { value = inverse ? value * 0.1 : value * 10; }
    }
    return negative ? -value : value;
}
