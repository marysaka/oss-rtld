#pragma once

#include <assert.h>
#include <elf.h>

#include "module_header.h"
#include "module_object.hpp"
#include "sdk_init.hpp"
#include "svc.h"
#include "utils.hpp"

struct module_object_list_t {
    module_object_t *front;
    module_object_t *back;
};

static_assert(sizeof(module_object_list_t) == 0x10,
              "module_object_list_t isn't valid");

uint64_t elf_hash(const unsigned char *name);
Elf64_Sym *module_get_sym(module_object_t *module_object, const char *name);

typedef Elf64_Addr (*lookup_global_t)(const char *);

Elf64_Addr lookup_global_auto(const char *name);
void resolve_symbols(module_object_t *module_object, bool do_lazy_got_init);

extern module_object_list_t g_pManualLoadList;
extern module_object_list_t g_pAutoLoadList;
extern bool g_RoDebugFlag;
extern lookup_global_t g_LookupGlobalManualFunctionPointer;
extern bool g_IsExceptionHandlerReady;