#include "sdk_init.hpp"

extern "C" void __rtld_start_app(Handle thread_handle,
                                 uintptr_t argument_address,
                                 void (*notify_exception_handler_ready)(),
                                 void (*call_initializator)()) {
    // Is is an old SDK?
    if (!nn::init::Start) {
        __nnDetailInitLibc0();

        nnosInitialize(thread_handle, argument_address);

        notify_exception_handler_ready();

        __nnDetailInitLibc1();

        nndiagStartup();

        if (nninitInitializeSdkModule) nninitInitializeSdkModule();

        if (nninitInitializeAbortObserver) nninitInitializeAbortObserver();

        nninitStartup();

        __nnDetailInitLibc2();
        call_initializator();
        nnMain();

        if (nninitFinalizeSdkModule) nninitFinalizeSdkModule();

        nnosQuickExit();
        svcExitProcess();

    } else {
        nn::init::Start(thread_handle, argument_address,
                        notify_exception_handler_ready, call_initializator);
    }
}