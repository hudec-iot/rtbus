/* SPDX-License-Identifier: MPL-2.0 */
#ifndef RTBUS_RUNTIME_POWER_H_
#define RTBUS_RUNTIME_POWER_H_

/* Runtime application power policy, called from RTBus task context.
 * Success never returns; failure is negative errno.
 */
int rtbus_runtime_power_off(void);

#endif
