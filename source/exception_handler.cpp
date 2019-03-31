#include "rtld.hpp"
#include "sdk_init.hpp"

namespace nn {
namespace os {
namespace detail {
// FIXME: We set this weak to avoid the compiler to optimize the check we have.
void WEAK UserExceptionHandler(void);
}  // namespace detail
}  // namespace os
}  // namespace nn

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