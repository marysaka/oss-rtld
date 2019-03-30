#include <stdarg.h>
#include <cstdio>
#include <cstring>
#include "rtld.hpp"

void debug_printf(const char *format, ...) {
    char log_buf[0x1000];
    va_list args;

    va_start(args, format);
    vsnprintf(log_buf, 0x1000, format, args);
    va_end(args);

    svcOutputDebugString(log_buf, strlen(log_buf));
}