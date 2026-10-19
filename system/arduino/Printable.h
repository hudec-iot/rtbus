/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stddef.h>
class Print;
class Printable {
public:
    virtual size_t printTo(Print &output) const = 0;
};
