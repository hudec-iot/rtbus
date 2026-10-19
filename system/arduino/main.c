/* SPDX-License-Identifier: MPL-2.0 */
#include "runtime_api.h"

void arduino_run_constructors(void);
void arduino_setup(void);
void arduino_loop(void);

int main(void)
{
    if (!rtbus_api_is_ready()) {
        return -RTBUS_ENOSYS;
    }

    arduino_run_constructors();
    arduino_setup();

    while (1) {
        arduino_loop();
    }
}
