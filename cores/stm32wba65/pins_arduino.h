/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

/* Arduino may search the core include directory before the selected variant. */
#if __has_include_next("pins_arduino.h")
#include_next "pins_arduino.h"
#endif
