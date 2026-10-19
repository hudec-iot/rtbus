/* SPDX-License-Identifier: MPL-2.0 */
#include <Arduino.h>

extern "C" void arduino_setup(void) {
    setup();
}

extern "C" void arduino_loop(void) {
    loop();
}

extern "C" {
typedef void (*Constructor)(void);
extern Constructor __rtduo_init_array_start[];
extern Constructor __rtduo_init_array_end[];
}

extern "C" void arduino_run_constructors(void)
{
    for (Constructor *entry = __rtduo_init_array_start; entry != __rtduo_init_array_end; ++entry) {
        (*entry)();
    }
}

extern "C" void __attribute__((weak)) __cxa_pure_virtual(void)
{
    __builtin_trap();
}

// Native applications do not exit through a libc process teardown. Global
// objects live until an upgrade replaces the image and resets its RAM.
extern "C" {
void *__attribute__((weak)) __dso_handle = nullptr;
int __attribute__((weak)) __aeabi_atexit(void *, void (*)(void *), void *) { return 0; }
int __attribute__((weak)) __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
}
