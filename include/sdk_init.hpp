#pragma once

#include "utils.hpp"

namespace nn {
namespace os {
namespace detail {
// FIXME: We set this weak to avoid a compiler optimization on a call.
void WEAK UserExceptionHandler(void);
}  // namespace detail
}  // namespace os

namespace init {
void WEAK Start(unsigned long thread_handle, unsigned long argument_address,
                void (*notify_exception_handler_ready)(),
                void (*call_initializator)());
}

}  // namespace nn

extern "C" void __nnDetailInitLibc0(void);
extern "C" void nnosInitialize(unsigned long thread_handle,
                               unsigned long argument_address);
extern "C" void __nnDetailInitLibc1(void);
extern "C" void __nnDetailInitLibc2(void);
extern "C" void nndiagStartup(void);
extern "C" void nninitStartup(void);
extern "C" void nnMain(void);
extern "C" void nnosQuickExit(void);

extern "C" WEAK void nninitInitializeSdkModule(void);
extern "C" WEAK void nninitInitializeAbortObserver(void);
extern "C" WEAK void nninitFinalizeSdkModule(void);