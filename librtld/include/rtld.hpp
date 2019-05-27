#pragma once

#include <elf.h>

#include <target_config.h>

#include "rtld/ModuleHeader.hpp"
#include "rtld/ModuleList.hpp"
#include "rtld/ModuleObject.hpp"
#include "rtld/svc.hpp"
#include "rtld/types.hpp"

using namespace rtld;

typedef Elf_Addr (*lookup_global_t)(const char *);

extern "C" void __rtld_runtime_resolve(void);

// Application entrypoint
extern "C" void __rtld_start_app(Handle thread_handle,
                                 uintptr_t argument_address,
                                 void (*notify_exception_handler_ready)(),
                                 void (*call_initializator)());

// Usermode exception entrypoint
extern "C" void __rtld_handle_exception();

namespace rtld {
Elf_Addr lookup_global_auto(const char *name);
extern ModuleObjectList g_pManualLoadList;
extern ModuleObjectList g_pAutoLoadList;
extern bool g_RoDebugFlag;
extern lookup_global_t g_LookupGlobalManualFunctionPointer;
extern bool g_IsExceptionHandlerReady;
}  // namespace rtld
