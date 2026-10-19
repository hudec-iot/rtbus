/* SPDX-License-Identifier: MPL-2.0 */
#include <stdint.h>

#include "runtime_api.h"

void delay(unsigned long ms)
{
    (void)k_delay((uint32_t)ms);
}

unsigned long millis(void)
{
    return (unsigned long)rtbus_millis();
}

void delayMicroseconds(unsigned int us)
{
    rtbus_delay_us((uint32_t)us);
}

void noInterrupts(void)
{
    rtbus_irq_disable();
}

void interrupts(void)
{
    rtbus_irq_restore();
}
