#include "rtld.hpp"
#include "sdk_init.hpp"

bool g_IsExceptionHandlerReady;

extern "C" void __rtld_notify_exception_handler_ready(void) {
    g_IsExceptionHandlerReady = true;
}

extern "C" void __rtld_handle_exception() {
    // TODO: manage it correctly when everything has been relocated
    if (!nn::os::detail::UserExceptionHandler) {
        svcReturnFromException(0xF801);
        while (true) {
        }
    }

    nn::os::detail::UserExceptionHandler();
}