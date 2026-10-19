/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef bool boolean;

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define lowByte(value) ((uint8_t)((value) & 0xff))
#define highByte(value) ((uint8_t)(((value) >> 8) & 0xff))
#define bit(index) (1UL << (index))
#define bitRead(value, index) (((value) >> (index)) & 1UL)
#define bitSet(value, index) ((value) |= bit(index))
#define bitClear(value, index) ((value) &= ~bit(index))
#define bitToggle(value, index) ((value) ^= bit(index))
#define bitWrite(value, index, state) ((state) ? bitSet(value, index) : bitClear(value, index))

#ifdef __cplusplus
// Function templates evaluate arguments once, including expressions with side effects.
template<typename A, typename B>
inline auto min(A a, B b) { return a < b ? a : b; }
template<typename A, typename B>
inline auto max(A a, B b) { return a > b ? a : b; }
template<typename T>
inline T abs(T value) { return value < 0 ? -value : value; }
template<typename T, typename L, typename H>
inline auto constrain(T value, L low, H high) {
    return value < low ? low : (value > high ? high : value);
}
template<typename T> inline T sq(T value) { return value * value; }
template<typename T> inline double radians(T value) { return value * DEG_TO_RAD; }
template<typename T> inline double degrees(T value) { return value * RAD_TO_DEG; }

typedef uint16_t word;
inline uint16_t makeWord(uint16_t value) { return value; }
inline uint16_t makeWord(uint8_t high, uint8_t low) { return ((uint16_t)high << 8) | low; }
#define word(...) makeWord(__VA_ARGS__)

long map(long value, long inLow, long inHigh, long outLow, long outHigh);
long random(long upper);
long random(long lower, long upper);
void randomSeed(unsigned long seed);
#endif

// ASCII character classification, independent of the runtime libc and locale.
static inline bool isAscii(int c) { return c >= 0 && c <= 127; }
static inline bool isAlpha(int c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static inline bool isDigit(int c) { return c >= '0' && c <= '9'; }
static inline bool isAlphaNumeric(int c) { return isAlpha(c) || isDigit(c); }
static inline bool isControl(int c) { return (c >= 0 && c < 32) || c == 127; }
static inline bool isGraph(int c) { return c >= 33 && c <= 126; }
static inline bool isHexadecimalDigit(int c) { return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
static inline bool isLowerCase(int c) { return c >= 'a' && c <= 'z'; }
static inline bool isUpperCase(int c) { return c >= 'A' && c <= 'Z'; }
static inline bool isPrintable(int c) { return c >= 32 && c <= 126; }
static inline bool isPunct(int c) { return isGraph(c) && !isAlphaNumeric(c); }
static inline bool isSpace(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
static inline bool isWhitespace(int c) { return c == ' ' || c == '\t'; }
