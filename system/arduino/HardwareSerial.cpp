/* SPDX-License-Identifier: MPL-2.0 */
#include "HardwareSerial.h"

#include <stdarg.h>

#include <runtime_api.h>

extern "C" size_t __attribute__((weak)) runtime_arduino_serial_write(uint32_t port,
                                                                     const uint8_t *data,
                                                                     size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }

    return rtbus_serial_write(port, data, size);
}

extern "C" int __attribute__((weak)) runtime_arduino_serial_begin(uint32_t port,
                                                                  uint32_t baud) {
    return rtbus_serial_control(port, RTBUS_SERIAL_CONTROL_BEGIN, baud, SERIAL_8N1);
}

extern "C" size_t __attribute__((weak)) runtime_arduino_serial_read(uint32_t port,
                                                                    uint8_t *data,
                                                                    size_t size) {
    size_t received = 0;

    if (data == nullptr || size == 0) {
        return 0;
    }

    while (received < size) {
        int32_t value = rtbus_serial_read(port);

        if (value < 0) {
            break;
        }

        data[received++] = static_cast<uint8_t>(value);
    }

    return received;
}

HardwareSerial Serial(HardwareSerial::PortSerial);
HardwareSerial Serial1(HardwareSerial::PortSerial1);

struct SerialInstance {
    uint32_t port;
    int peeked;
};

static SerialInstance serial_instances[] = {
    {RTBUS_SERIAL_PORT_0, -1},
    {RTBUS_SERIAL_PORT_1, -1},
};

void HardwareSerial::begin(unsigned long baud)
{
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    instance->peeked = -1;
    lastError_ = runtime_arduino_serial_begin(instance->port, (uint32_t)baud);
    started_ = lastError_ == 0;
}

void HardwareSerial::begin(unsigned long baud, uint16_t config)
{
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    instance->peeked = -1;
    lastError_ = rtbus_serial_control(instance->port, RTBUS_SERIAL_CONTROL_BEGIN, (uint32_t)baud, config);
    started_ = lastError_ == 0;
}

void HardwareSerial::end()
{
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    int drained = started_ ? rtbus_serial_control(instance->port, RTBUS_SERIAL_CONTROL_FLUSH, 0, 0) : 0;
    int ended = rtbus_serial_control(instance->port, RTBUS_SERIAL_CONTROL_END, 0, 0);
    lastError_ = ended != 0 ? ended : drained;
    instance->peeked = -1;
    started_ = false;
}

void HardwareSerial::flush()
{
    lastError_ = rtbus_serial_control((uint32_t)port_, RTBUS_SERIAL_CONTROL_FLUSH, 0, 0);
}

int HardwareSerial::availableForWrite()
{
    // Writes use the runtime's synchronous polling path, not an app TX queue.
    return 0;
}

HardwareSerial::operator bool()
{
    if (!started_) { return false; }
    int ready = rtbus_serial_control((uint32_t)port_, RTBUS_SERIAL_CONTROL_READY, 0, 0);
    if (ready < 0) { lastError_ = ready; }
    return ready > 0;
}

int HardwareSerial::available()
{
    if (!started_) { return 0; }
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    int count = rtbus_serial_control(instance->port, RTBUS_SERIAL_CONTROL_AVAILABLE, 0, 0);
    if (count < 0) { lastError_ = count; return instance->peeked >= 0 ? 1 : 0; }
    return count + (instance->peeked >= 0 ? 1 : 0);
}

int HardwareSerial::peek()
{
    if (!started_) { return -1; }
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    if (instance->peeked < 0) {
        uint8_t byte;
        if (runtime_arduino_serial_read(instance->port, &byte, 1) == 0) { return -1; }
        instance->peeked = byte;
    }
    return instance->peeked;
}

int HardwareSerial::read(void) {
    if (!started_) { return -1; }
    uint8_t byte;
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];

    if (instance->peeked >= 0) {
        int value = instance->peeked;

        instance->peeked = -1;
        return value;
    }

    if (runtime_arduino_serial_read(instance->port, &byte, 1) == 0) {
        return -1;
    }

    return byte;
}

size_t HardwareSerial::write(uint8_t value) {
    return write(&value, 1);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];

    if (!started_ || buffer == nullptr || size == 0) {
        return 0;
    }

    size_t written = runtime_arduino_serial_write(instance->port, buffer, size);
    if (written != size) { setWriteError(); }
    return written;
}

size_t HardwareSerial::printf(const char *fmt, ...) {
    va_list args;
    SerialInstance *instance = &serial_instances[static_cast<size_t>(port_)];
    size_t written = 0;

    if (!started_ || fmt == nullptr) {
        return 0;
    }

    va_start(args, fmt);
    written = rtbus_serial_vprintf(instance->port, fmt, args);
    va_end(args);

    return written;
}


