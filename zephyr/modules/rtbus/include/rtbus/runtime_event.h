/*
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef RTBUS_RUNTIME_EVENT_H_
#define RTBUS_RUNTIME_EVENT_H_

#include <stdint.h>

void runtime_event_signal(void);
/* Negative timeout waits indefinitely; zero checks without blocking. */
int runtime_event_wait(int32_t timeout_ms);

#endif /* RTBUS_RUNTIME_EVENT_H_ */
