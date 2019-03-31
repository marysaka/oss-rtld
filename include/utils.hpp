#pragma once

#include <cstring>

#include "svc.h"

#define WEAK __attribute__((weak))

inline void debug_print(const char *str) {
    svcOutputDebugString(str, strlen(str));
}
