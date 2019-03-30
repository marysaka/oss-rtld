#pragma once
#include <elf.h>
#include <stddef.h>
#include <stdint.h>

#include <cstring>

typedef uint64_t Result;

extern "C" {
#define WEAK __attribute__((weak))
#define MOD0_MAGIC 0x30444F4D

typedef struct {
    uint32_t magic;
    uint32_t dynamic_offset;
    uint32_t bss_start_offset;
    uint32_t bss_end_offset;
    uint32_t unwind_start_offset;
    uint32_t unwind_end_offset;
    uint32_t module_object_offset;
} module_header_t;

typedef struct module_object_s {
    struct module_object_s *next;
    struct module_object_s *prev;
    union {
        Elf64_Rel *rel;
        Elf64_Rela *rela;
        void *raw;
    } rela_or_rel_plt;
    union {
        Elf64_Rel *rel;
        Elf64_Rela *rela;
    } rela_or_rel;
    uint64_t module_base;
    Elf64_Dyn *dyanmic;
    bool is_rela;
    uint64_t rela_or_rel_plt_size;
    void (*dt_init)(void);
    void (*dt_fini)(void);
    uint32_t *hash_bucket;
    uint32_t *hash_chain;
    char *dynstr;
    Elf64_Sym *dynsym;
    uint64_t dynstr_size;
    void **got;
    uint64_t rela_dyn_size;
    uint64_t rel_dyn_size;
    uint64_t rel_count;
    uint64_t rela_count;
    uint64_t hash_nchain_value;
    uint64_t hash_nbucket_value;
    uint64_t got_stub_ptr;
} module_object_t;

_Static_assert(sizeof(module_object_t) == 0xB8,
               "module_object_t size isn't valid");

typedef struct {
    module_object_t *front;
    module_object_t *back;
} module_object_list_t;

typedef struct {
    uint64_t address;
    uint64_t size;
    uint32_t type;
    uint32_t attribute;
    uint32_t permission;
    uint32_t device_ref_count;
    uint32_t ipc_ref_count;
    uint32_t padding;
} memory_info_t;
_Static_assert(sizeof(memory_info_t) == 0x28, "memory_info_t size isn't valid");

Result svcQueryMemory(memory_info_t *memory_info_ptr, uint32_t *pageinfo,
                      uint64_t addr);
Result svcBreak(uint32_t breakReason, uint64_t unk, uint64_t info);
void svcReturnFromException(Result errorCode);
Result svcOutputDebugString(const char *str, size_t str_size);
}

inline void debug_print(const char *str) {
    svcOutputDebugString(str, strlen(str));
}

void debug_printf(const char *format, ...);

uint64_t elf_hash(const unsigned char *name);
Elf64_Sym *module_get_sym(module_object_t *module_object, const char *name);

typedef Elf64_Addr (*lookup_global_t)(const char *);

Elf64_Addr lookup_global_auto(const char *name);

void resolve_symbols(module_object_t *module_object, bool do_lazy_got_init);

extern module_object_list_t g_pManualLoadList;
extern module_object_list_t g_pAutoLoadList;
extern bool g_RoDebugFlag;
extern lookup_global_t g_LookupGlobalManualFunctionPointer;