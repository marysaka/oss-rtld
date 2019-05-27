#include "sdk_init.hpp"

#define APPLICATION_NAME "oss-rtld"
#define APPLICATION_NAME_LEN 8

struct ApplicationName {
    int unknown;
    int name_lengh;
    char name[APPLICATION_NAME_LEN + 1];
};

__attribute__((section(".rodata.application_name")))
ApplicationName application_name = {
    .unknown = 0, .name_lengh = APPLICATION_NAME_LEN, .name = APPLICATION_NAME};

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
        rtld::svc::ExitProcess();

    } else {
        nn::init::Start(thread_handle, argument_address,
                        notify_exception_handler_ready, call_initializator);
    }
}

extern "C" void __rtld_handle_exception() {
    if (!nn::os::detail::UserExceptionHandler) {
        rtld::svc::ReturnFromException(0xF801);
        while (true) {
        }
    }

    nn::os::detail::UserExceptionHandler();
}
