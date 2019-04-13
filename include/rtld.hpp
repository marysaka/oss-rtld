#pragma once

#include <assert.h>
#include <elf.h>

#include "rtld/ModuleHeader.hpp"
#include "rtld/ModuleObject.hpp"
#include "sdk_init.hpp"
#include "svc.h"
#include "utils.hpp"

using namespace rtld;

struct module_object_list_t {
    ModuleObject *front;
    ModuleObject *back;
};

static_assert(sizeof(module_object_list_t) == 0x10,
              "module_object_list_t isn't valid");

typedef Elf64_Addr (*lookup_global_t)(const char *);

Elf64_Addr lookup_global_auto(const char *name);
extern module_object_list_t g_pManualLoadList;
extern module_object_list_t g_pAutoLoadList;
extern bool g_RoDebugFlag;
extern lookup_global_t g_LookupGlobalManualFunctionPointer;
extern bool g_IsExceptionHandlerReady;
extern "C" void __rtld_runtime_resolve(void);
