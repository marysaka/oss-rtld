#include "rtld.hpp"

extern "C" void handle_exception(Result error_code) {
    // TODO: manage it correctly when everything has been relocated
    (void)error_code;
    svcReturnFromException(0xF801);
}