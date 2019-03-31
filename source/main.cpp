#include "rtld.hpp"

bool g_IsExceptionHandlerReady;

// On the real rtld, those are stubbed.
extern "C" void __rtld_init(void) {}

extern "C" void __rtld_fini(void) {}

extern "C" void set_exception_handler_ready(void) {
    g_IsExceptionHandlerReady = true;
}