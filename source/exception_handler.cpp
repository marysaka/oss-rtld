#include "rtld.hpp"

namespace nn {
namespace os {
namespace detail {
// FIXME: We set this weak to avoid the compiler to optimize the check we have.
void WEAK UserExceptionHandler(void);
}  // namespace detail
}  // namespace os
}  // namespace nn

extern "C" void handle_exception() {
    // TODO: manage it correctly when everything has been relocated
    if (!nn::os::detail::UserExceptionHandler) {
        svcReturnFromException(0xF801);
        while (true) {
        }
    }

    nn::os::detail::UserExceptionHandler();
}