/* SPDX-License-Identifier: MPL-2.0 */
#include <Arduino.h>

extern "C" void arduino_setup(void) {
    setup();
}

extern "C" void arduino_loop(void) {
    loop();
}
