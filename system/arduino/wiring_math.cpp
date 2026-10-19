/* SPDX-License-Identifier: MPL-2.0 */
// Included by each core's wiring_math.cpp. No libc allocator or RNG is needed.
#include <stdint.h>

static uint32_t arduino_random_state = 1;

void randomSeed(unsigned long seed)
{
    if (seed != 0) {
        arduino_random_state = (uint32_t)(seed % 2147483647UL);
        if (arduino_random_state == 0) {
            arduino_random_state = 1;
        }
    }
}

long random(long upper)
{
    if (upper <= 0) {
        return 0;
    }
    // Park-Miller generator; the intermediate fits in uint64_t.
    arduino_random_state = (uint32_t)(((uint64_t)arduino_random_state * 16807U) % 2147483647U);
    return (long)(arduino_random_state % (uint32_t)upper);
}

long random(long lower, long upper)
{
    if (lower >= upper) {
        return lower;
    }
    // Unsigned subtraction handles intervals spanning LONG_MIN to LONG_MAX.
    uint32_t width = (uint32_t)upper - (uint32_t)lower;
    arduino_random_state = (uint32_t)(((uint64_t)arduino_random_state * 16807U) % 2147483647U);
    return (long)((int64_t)lower + arduino_random_state % width);
}

long map(long value, long inLow, long inHigh, long outLow, long outHigh)
{
    // Like Arduino map(), callers must supply distinct input endpoints.
    return (value - inLow) * (outHigh - outLow) / (inHigh - inLow) + outLow;
}
