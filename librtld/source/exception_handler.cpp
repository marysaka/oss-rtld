#include "rtld.hpp"

bool rtld::g_IsExceptionHandlerReady;

extern "C" void __rtld_notify_exception_handler_ready(void) {
    g_IsExceptionHandlerReady = true;
}
