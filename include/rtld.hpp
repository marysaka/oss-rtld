#pragma once

#include <assert.h>
#include <elf.h>

#include "rtld/ModuleHeader.hpp"
#include "rtld/ModuleList.hpp"
#include "rtld/ModuleObject.hpp"
#include "sdk_init.hpp"
#include "svc.h"
#include "utils.hpp"

using namespace rtld;

typedef Elf64_Addr (*lookup_global_t)(const char *);

extern "C" void __rtld_runtime_resolve(void);

namespace rtld {
Elf64_Addr lookup_global_auto(const char *name);
extern ModuleObjectList g_pManualLoadList;
extern ModuleObjectList g_pAutoLoadList;
extern bool g_RoDebugFlag;
extern lookup_global_t g_LookupGlobalManualFunctionPointer;
extern bool g_IsExceptionHandlerReady;
}  // namespace rtld
