#include "rtld.hpp"

namespace nn {
    namespace init {
        void WEAK Start(unsigned long thread_handle, unsigned long argument_address, void (*notify_exception_handler_ready)(), void (*call_initializator)());
    }
}

extern "C" void __nnDetailInitLibc0(void);
extern "C" void nnosInitialize(unsigned long thread_handle, unsigned long argument_address);
extern "C" void __nnDetailInitLibc1(void);
extern "C" void __nnDetailInitLibc2(void);
extern "C" void nndiagStartup(void);
extern "C" void nninitStartup(void);
extern "C" void nnMain(void);
extern "C" void nnosQuickExit(void);

extern "C" WEAK void nninitInitializeSdkModule(void);
extern "C" WEAK void nninitInitializeAbortObserver(void);
extern "C" WEAK void nninitFinalizeSdkModule(void);

bool g_IsExceptionHandlerReady;

// On the real rtld, those are stubbed.
extern "C" void __rtld_init(void) {}

extern "C" void __rtld_fini(void) {}

extern "C" void __rtld_notify_exception_handler_ready(void) {
    g_IsExceptionHandlerReady = true;
}

extern "C" void __rtld_start_app(unsigned long thread_handle, unsigned long argument_address, void (*notify_exception_handler_ready)(), void (*call_initializator)()) {
    // Is is an old SDK?
    if (!nn::init::Start) {
        __nnDetailInitLibc0();
        nnosInitialize(thread_handle, argument_address);

        notify_exception_handler_ready();

        __nnDetailInitLibc1();
        nndiagStartup();

        if (nninitInitializeSdkModule)
            nninitInitializeSdkModule();

        if (nninitInitializeAbortObserver)
            nninitInitializeAbortObserver();

        nninitStartup();

        __nnDetailInitLibc2();
        call_initializator();
        nnMain();

        if (nninitFinalizeSdkModule)
            nninitFinalizeSdkModule();
        
        nnosQuickExit();
        svcExitProcess();

    } else {
        nn::init::Start(thread_handle, argument_address, notify_exception_handler_ready, call_initializator);
    }
}