/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include "Print.h"
#include "WString.h"
enum LookaheadMode { SKIP_ALL, SKIP_NONE, SKIP_WHITESPACE };
class Stream : public Print {
protected:
    unsigned long timeout_ = 1000;
    int timedRead();
    int timedPeek();
    int peekNumber(LookaheadMode mode, bool decimal);
    long parseInt(char ignore) { return parseInt(SKIP_ALL, ignore); }
    float parseFloat(char ignore) { return parseFloat(SKIP_ALL, ignore); }
    struct MultiTarget { const char *str; size_t len; size_t index; };
    int findMulti(MultiTarget *targets, int count);
public:
    constexpr Stream() = default;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    void setTimeout(unsigned long value) { timeout_ = value; }
    unsigned long getTimeout() const { return timeout_; }
    size_t readBytes(char *buffer, size_t length);
    size_t readBytes(uint8_t *buffer, size_t length) { return readBytes((char *)buffer, length); }
    size_t readBytesUntil(char terminator, char *buffer, size_t length);
    size_t readBytesUntil(char terminator, uint8_t *buffer, size_t length) { return readBytesUntil(terminator, (char *)buffer, length); }
    bool find(const char *target);
    bool find(const char *target, size_t length);
    bool find(char target) { return find(&target, 1); }
    bool find(const uint8_t *target) { return find((const char *)target); }
    bool find(const uint8_t *target, size_t length) { return find((const char *)target, length); }
    bool findUntil(const char *target, const char *terminator);
    bool findUntil(const char *target, size_t length, const char *terminator, size_t terminatorLength);
    bool findUntil(const uint8_t *target, const char *terminator) { return findUntil((const char *)target, terminator); }
    bool findUntil(const uint8_t *target, size_t length, const char *terminator, size_t terminatorLength) { return findUntil((const char *)target, length, terminator, terminatorLength); }
    long parseInt(LookaheadMode mode = SKIP_ALL, char ignore = '\x01');
    float parseFloat(LookaheadMode mode = SKIP_ALL, char ignore = '\x01');
    String readString();
    String readStringUntil(char terminator);
};
