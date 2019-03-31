#pragma once

#include <assert.h>
#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// TODO: switch to a C++ class.
struct module_object_t {
    struct module_object_t *next;
    struct module_object_t *prev;
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
};

static_assert(sizeof(module_object_t) == 0xB8,
              "module_object_t size isn't valid");
