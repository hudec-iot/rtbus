/* SPDX-License-Identifier: MPL-2.0 */
#include "Stream.h"
#include "Arduino.h"

int Stream::timedRead()
{
    unsigned long start = millis();
    do { int c = read(); if (c >= 0) { return c; } yield(); }
    while ((unsigned long)(millis() - start) < timeout_);
    return -1;
}
int Stream::timedPeek()
{
    unsigned long start = millis();
    do { int c = peek(); if (c >= 0) { return c; } yield(); }
    while ((unsigned long)(millis() - start) < timeout_);
    return -1;
}
size_t Stream::readBytes(char *buffer, size_t length)
{
    size_t n = 0;
    if (!buffer) { return 0; }
    while (n < length) { int c = timedRead(); if (c < 0) { break; } buffer[n++] = (char)c; }
    return n;
}
size_t Stream::readBytesUntil(char terminator, char *buffer, size_t length)
{
    size_t n = 0;
    if (!buffer) { return 0; }
    while (n < length) { int c = timedRead(); if (c < 0 || c == terminator) { break; } buffer[n++] = (char)c; }
    return n;
}
static size_t textLength(const char *text)
{
    size_t n = 0; if (text) { while (text[n]) { ++n; } } return n;
}
// Prefix fallback preserves overlapping matches without allocating a table.
static size_t matchByte(const char *pattern, size_t matched, char c)
{
    while (matched && pattern[matched] != c) {
        size_t next = matched - 1;
        for (; next; --next) {
            size_t i = 0;
            while (i < next && pattern[i] == pattern[matched - next + i]) { ++i; }
            if (i == next) { break; }
        }
        matched = next;
    }
    return pattern[matched] == c ? matched + 1 : 0;
}
bool Stream::find(const char *target) { return find(target, textLength(target)); }
int Stream::findMulti(MultiTarget *targets, int count)
{
    if (!targets || count <= 0) { return -1; }
    for (int i = 0; i < count; ++i) {
        if (!targets[i].str) { return -1; }
        targets[i].index = 0;
        if (targets[i].len == 0) { return i; }
    }
    int c;
    while ((c = timedRead()) >= 0) {
        for (int i = 0; i < count; ++i) {
            targets[i].index = matchByte(targets[i].str, targets[i].index, (char)c);
            if (targets[i].index == targets[i].len) { return i; }
        }
    }
    return -1;
}
bool Stream::find(const char *target, size_t length) { return findUntil(target, length, nullptr, 0); }
bool Stream::findUntil(const char *target, const char *end) { return findUntil(target, textLength(target), end, textLength(end)); }
bool Stream::findUntil(const char *target, size_t length, const char *end, size_t endLength)
{
    if (!target || (!end && endLength)) { return false; }
    if (!length) { return true; }
    size_t matched = 0, ended = 0;
    int c;
    while ((c = timedRead()) >= 0) {
        matched = matchByte(target, matched, (char)c);
        if (matched == length) { return true; }
        if (endLength) { ended = matchByte(end, ended, (char)c); if (ended == endLength) { return false; } }
    }
    return false;
}
int Stream::peekNumber(LookaheadMode mode, bool decimal)
{
    int c;
    while ((c = timedPeek()) >= 0) {
        if (isDigit(c) || c == '-' || (decimal && c == '.')) { return c; }
        if (mode == SKIP_NONE || (mode == SKIP_WHITESPACE && !isSpace(c))) { return -1; }
        read();
    }
    return -1;
}
long Stream::parseInt(LookaheadMode mode, char ignore)
{
    int c = peekNumber(mode, false);
    if (c < 0) { return 0; }
    bool negative = false;
    unsigned long value = 0;
    do {
        if (c == ignore) { }
        else if (c == '-') { negative = true; }
        else { value = value * 10 + (unsigned)(c - '0'); }
        read(); c = timedPeek();
    } while (isDigit(c) || c == ignore);
    return (long)(negative ? 0UL - value : value);
}
float Stream::parseFloat(LookaheadMode mode, char ignore)
{
    int c = peekNumber(mode, true);
    if (c < 0) { return 0; }
    bool negative = false, decimal = false;
    double value = 0, factor = 1;
    do {
        if (c == ignore) { }
        else if (c == '-') { negative = true; }
        else if (c == '.') { decimal = true; }
        else { value = value * 10 + c - '0'; if (decimal) { factor *= 0.1; } }
        read(); c = timedPeek();
    } while (isDigit(c) || c == ignore || (c == '.' && !decimal));
    return (float)((negative ? -value : value) * factor);
}
String Stream::readString()
{
    String result; int c;
    while ((c = timedRead()) >= 0) { if (!result.concat((char)c)) { break; } }
    return result;
}
String Stream::readStringUntil(char terminator)
{
    String result; int c;
    while ((c = timedRead()) >= 0 && c != terminator) { if (!result.concat((char)c)) { break; } }
    return result;
}
