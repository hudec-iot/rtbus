/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "Stream.h"

#if defined(__GNUC__)
#define RTDUO_SERIAL_PRINTF_FORMAT __attribute__((format(printf, 2, 3)))
#else
#define RTDUO_SERIAL_PRINTF_FORMAT
#endif

#define SERIAL_PARITY_EVEN 0x1U
#define SERIAL_PARITY_ODD 0x2U
#define SERIAL_PARITY_NONE 0x3U
#define SERIAL_PARITY_MARK 0x4U
#define SERIAL_PARITY_SPACE 0x5U
#define SERIAL_PARITY_MASK 0xFU
#define SERIAL_STOP_BIT_1 0x10U
#define SERIAL_STOP_BIT_1_5 0x20U
#define SERIAL_STOP_BIT_2 0x30U
#define SERIAL_STOP_BIT_MASK 0xF0U
#define SERIAL_DATA_5 0x100U
#define SERIAL_DATA_6 0x200U
#define SERIAL_DATA_7 0x300U
#define SERIAL_DATA_8 0x400U
#define SERIAL_DATA_MASK 0xF00U

#define SERIAL_5N1 0x113U
#define SERIAL_5N2 0x133U
#define SERIAL_5E1 0x111U
#define SERIAL_5E2 0x131U
#define SERIAL_5O1 0x112U
#define SERIAL_5O2 0x132U
#define SERIAL_5M1 0x114U
#define SERIAL_5M2 0x134U
#define SERIAL_5S1 0x115U
#define SERIAL_5S2 0x135U
#define SERIAL_6N1 0x213U
#define SERIAL_6N2 0x233U
#define SERIAL_6E1 0x211U
#define SERIAL_6E2 0x231U
#define SERIAL_6O1 0x212U
#define SERIAL_6O2 0x232U
#define SERIAL_6M1 0x214U
#define SERIAL_6M2 0x234U
#define SERIAL_6S1 0x215U
#define SERIAL_6S2 0x235U
#define SERIAL_7N1 0x313U
#define SERIAL_7N2 0x333U
#define SERIAL_7E1 0x311U
#define SERIAL_7E2 0x331U
#define SERIAL_7O1 0x312U
#define SERIAL_7O2 0x332U
#define SERIAL_7M1 0x314U
#define SERIAL_7M2 0x334U
#define SERIAL_7S1 0x315U
#define SERIAL_7S2 0x335U
#define SERIAL_8N1 0x413U
#define SERIAL_8N2 0x433U
#define SERIAL_8E1 0x411U
#define SERIAL_8E2 0x431U
#define SERIAL_8O1 0x412U
#define SERIAL_8O2 0x432U
#define SERIAL_8M1 0x414U
#define SERIAL_8M2 0x434U
#define SERIAL_8S1 0x415U
#define SERIAL_8S2 0x435U

class HardwareSerial : public Stream {
public:
    enum Port {
        PortSerial = 0,
        PortSerial1 = 1,
    };

    constexpr explicit HardwareSerial(Port port = PortSerial) : port_(port) {
    }

    void begin(unsigned long baud);
    void end(void);

    int available(void);
    int peek(void);
    int read(void);
    void flush() override;
    int availableForWrite() override;
    using Print::write;
    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    size_t printf(const char *fmt, ...) RTDUO_SERIAL_PRINTF_FORMAT;
    void begin(unsigned long baud, uint16_t config);
    operator bool();
    int lastError() const { return lastError_; }

private:
    const Port port_;
    int lastError_ = 0;
    bool started_ = false;
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;

#undef RTDUO_SERIAL_PRINTF_FORMAT
