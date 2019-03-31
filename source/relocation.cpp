#include "rtld.hpp"

__attribute__((section(".bss"))) module_object_t __nx_module_runtime;
module_object_list_t g_pManualLoadList;
module_object_list_t g_pAutoLoadList;
bool g_RoDebugFlag;
lookup_global_t g_LookupGlobalManualFunctionPointer;

inline void relocate_self(uint64_t alsr_base, Elf64_Dyn *dynamic) {
    uint64_t rela = 0;
    uint64_t rel = 0;

    uint64_t rela_entry_size = sizeof(Elf64_Rela);
    uint64_t rel_entry_size = sizeof(Elf64_Rel);

    uint64_t rela_entry_count = 0;
    uint64_t rel_entry_count = 0;

    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        switch (dynamic->d_tag) {
            case DT_RELA:
                rela = (alsr_base + dynamic->d_un.d_ptr);
                break;

            case DT_RELAENT:
                rela_entry_size = dynamic->d_un.d_val;
                break;

            case DT_REL:
                rel = (alsr_base + dynamic->d_un.d_ptr);
                break;

            case DT_RELENT:
                rel_entry_size = dynamic->d_un.d_val;
                break;

            case DT_RELACOUNT:
                rela_entry_count = dynamic->d_un.d_val;
                break;

            case DT_RELCOUNT:
                rel_entry_count = dynamic->d_un.d_val;
                break;

            // those are nop on the real rtld
            case DT_NEEDED:
            case DT_PLTRELSZ:
            case DT_PLTGOT:
            case DT_HASH:
            case DT_STRTAB:
            case DT_SYMTAB:
            case DT_RELASZ:
            case DT_STRSZ:
            case DT_SYMENT:
            case DT_INIT:
            case DT_FINI:
            case DT_SONAME:
            case DT_RPATH:
            case DT_SYMBOLIC:
            case DT_RELSZ:
            default:
                break;
        }
    }

    if (rel_entry_count) {
        uint64_t i = 0;

        while (i < rel_entry_count) {
            Elf64_Rel *entry = (Elf64_Rel *)(rel + (i * rel_entry_size));
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr = (uint64_t *)(alsr_base + entry->r_offset);
                    *ptr += alsr_base;
                    break;
                }
            }
            i++;
        }
    }

    if (rela_entry_count) {
        uint64_t i = 0;

        while (i < rela_entry_count) {
            Elf64_Rela *entry = (Elf64_Rela *)(rela + (i * rela_entry_size));
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr = (uint64_t *)(alsr_base + entry->r_offset);
                    *ptr = alsr_base + entry->r_addend;
                    break;
                }
            }
            i++;
        }
    }
}

void initialize_module(module_object_t *module_object, uint64_t aslr_base,
                       Elf64_Dyn *dynamic) {
    module_object->module_base = aslr_base;
    module_object->dyanmic = dynamic;
    module_object->rela_or_rel_plt_size = 0;
    module_object->dt_init = nullptr;
    module_object->dt_fini = nullptr;
    module_object->hash_bucket = nullptr;
    module_object->hash_chain = nullptr;
    module_object->dynstr = nullptr;
    module_object->dynsym = nullptr;
    module_object->dynstr_size = 0;
    module_object->rela_or_rel.rela = nullptr;
    module_object->got = nullptr;
    module_object->rela_dyn_size = 0;
    module_object->rel_dyn_size = 0;
    module_object->got_stub_ptr = 0;

    void *rel_plt = nullptr;

    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        switch (dynamic->d_tag) {
            case DT_PLTRELSZ: {
                module_object->rela_or_rel_plt_size = dynamic->d_un.d_val;
                break;
            }

            case DT_PLTGOT: {
                module_object->got = (void **)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_HASH: {
                uint32_t *hash_table =
                    (uint32_t *)(aslr_base + dynamic->d_un.d_val);

                const uint32_t nbucket = hash_table[0];
                const uint32_t nchain = hash_table[1];

                module_object->hash_nbucket_value = nbucket;
                module_object->hash_nchain_value = nchain;
                module_object->hash_bucket = &hash_table[2];
                module_object->hash_chain = &hash_table[2 + nbucket];
                break;
            }

            case DT_STRTAB: {
                module_object->dynstr =
                    (char *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_SYMTAB: {
                module_object->dynsym =
                    (Elf64_Sym *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_REL: {
                module_object->rela_or_rel.rel =
                    (Elf64_Rel *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELA: {
                module_object->rela_or_rel.rela =
                    (Elf64_Rela *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELASZ: {
                module_object->rela_dyn_size = dynamic->d_un.d_val;
                break;
            }

            case DT_RELAENT: {
                if (dynamic->d_un.d_val != sizeof(Elf64_Rela)) {
                    svcBreak(0, 0, 0);
                }
                break;
            }

            case DT_SYMENT: {
                if (dynamic->d_un.d_val != sizeof(Elf64_Sym)) {
                    svcBreak(0, 0, 0);
                }
                break;
            }

            case DT_STRSZ: {
                module_object->dynstr_size = dynamic->d_un.d_val;
                break;
            }

            case DT_INIT: {
                module_object->dt_init =
                    (void (*)())(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_FINI: {
                module_object->dt_fini =
                    (void (*)())(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELSZ: {
                module_object->rel_dyn_size = dynamic->d_un.d_val;
                break;
            }

            case DT_RELENT: {
                if (dynamic->d_un.d_val != sizeof(Elf64_Rel)) {
                    svcBreak(0, 0, 0);
                }
                break;
            }

            case DT_PLTREL: {
                uint64_t value = dynamic->d_un.d_val;
                module_object->is_rela = value == DT_RELA;
                if (value != DT_REL && value != DT_RELA) {
                    svcBreak(0, 0, 0);
                }
                break;
            }

            case DT_JMPREL: {
                rel_plt = (void *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELACOUNT:
                module_object->rela_count = dynamic->d_un.d_val;
                break;

            case DT_RELCOUNT:
                module_object->rel_count = dynamic->d_un.d_val;
                break;

            case DT_NEEDED:
            case DT_SONAME:
            case DT_RPATH:
            case DT_SYMBOLIC:
            case DT_DEBUG:
            case DT_TEXTREL:
            default:
                break;
        }
    }

    module_object->rela_or_rel_plt.raw = rel_plt;
}

void relocate_module(module_object_t *module_object) {
    if (module_object->rela_count) {
        for (uint64_t i = 0; i < module_object->rel_count; i++) {
            Elf64_Rel *entry = &module_object->rela_or_rel.rel[i];
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr = (uint64_t *)(module_object->module_base +
                                                 entry->r_offset);
                    *ptr += module_object->module_base;
                    break;
                }
            }
        }
    }

    if (module_object->rela_count) {
        for (uint64_t i = 0; i < module_object->rela_count; i++) {
            Elf64_Rela *entry = &module_object->rela_or_rel.rela[i];
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr = (uint64_t *)(module_object->module_base +
                                                 entry->r_offset);
                    *ptr = module_object->module_base + entry->r_addend;
                    break;
                }
            }
        }
    }
}

extern "C" void __rtld_relocate_modules(uint64_t aslr_base,
                                        Elf64_Dyn *dynamic) {
    relocate_self(aslr_base, dynamic);

    // Init global lists
    // Those are only used to know the end and the start of the list
    g_pManualLoadList.front = (module_object_t *)&g_pManualLoadList;
    g_pManualLoadList.back = (module_object_t *)&g_pManualLoadList;

    g_pAutoLoadList.front = (module_object_t *)&g_pAutoLoadList;
    g_pAutoLoadList.back = (module_object_t *)&g_pAutoLoadList;

    __nx_module_runtime.prev = &__nx_module_runtime;
    __nx_module_runtime.next = &__nx_module_runtime;

    initialize_module(&__nx_module_runtime, aslr_base, dynamic);

    module_object_t *next_module = __nx_module_runtime.next;
    module_object_t *front_module = g_pAutoLoadList.front;

    __nx_module_runtime.next = front_module;
    next_module->prev = (module_object_t *)&g_pAutoLoadList;
    front_module->prev = &__nx_module_runtime;
    g_pAutoLoadList.front = next_module;

    uint64_t last_address = 0;
    memory_info_t memory_info;
    uint32_t dummy;

    bool is_past_last_address;
    do {
        Result res = svcQueryMemory(&memory_info, &dummy, last_address);
        if (res) {
            while (true) {
            }
        }

        if (memory_info.permission == 5 && memory_info.type == 3 &&
            memory_info.address != aslr_base) {
            char *module_header_address =
                (char *)(memory_info.address +
                         *(uint32_t *)(memory_info.address + 4));
            module_header_t *module_header =
                (module_header_t *)module_header_address;
            if (module_header->magic != MOD0_MAGIC) {
                while (true) {
                }
            }

            if (module_header->bss_start_offset !=
                module_header->bss_end_offset) {
                memset(module_header_address + module_header->bss_start_offset,
                       0,
                       module_header->bss_end_offset -
                           module_header->bss_start_offset);
            }

            module_object_t *module_object =
                (module_object_t *)(module_header_address +
                                    module_header->module_object_offset);
            Elf64_Dyn *module_dynamic =
                (Elf64_Dyn *)(module_header_address +
                              module_header->dynamic_offset);

            module_object->next = module_object;
            module_object->prev = module_object;
            initialize_module(module_object, memory_info.address,
                              module_dynamic);
            relocate_module(module_object);

            next_module = module_object->next;
            module_object->next = g_pAutoLoadList.front;
            next_module->prev = (module_object_t *)&g_pAutoLoadList;
            g_pAutoLoadList.front->prev = module_object;
            g_pAutoLoadList.front = next_module;
        }

        is_past_last_address =
            memory_info.address + memory_info.size > last_address;
        last_address = memory_info.address + memory_info.size;
    } while (is_past_last_address);

    if (g_pAutoLoadList.back == (module_object_t *)&g_pAutoLoadList) {
        return;
    }

    for (module_object_t *module = g_pAutoLoadList.back;
         module != (module_object_t *)&g_pAutoLoadList; module = module->prev) {
        Elf64_Sym *symbol =
            module_get_sym(module, "_ZN2nn2ro6detail15g_pAutoLoadListE");
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            module_object_list_t **symbol_val =
                (module_object_list_t **)(module->module_base +
                                          symbol->st_value);
            *symbol_val = &g_pAutoLoadList;
        }

        symbol = module_get_sym(module, "_ZN2nn2ro6detail17g_pManualLoadListE");
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            module_object_list_t **symbol_val =
                (module_object_list_t **)(module->module_base +
                                          symbol->st_value);
            *symbol_val = &g_pManualLoadList;
        }

        symbol = module_get_sym(module, "_ZN2nn2ro6detail14g_pRoDebugFlagE");
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            bool **symbol_val =
                (bool **)(module->module_base + symbol->st_value);
            *symbol_val = &g_RoDebugFlag;
        }

        symbol = module_get_sym(
            module, "_ZN2nn2ro6detail34g_pLookupGlobalAutoFunctionPointerE");
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            lookup_global_t *symbol_val =
                (lookup_global_t *)(module->module_base + symbol->st_value);
            *symbol_val = lookup_global_auto;
        }

        symbol = module_get_sym(
            module, "_ZN2nn2ro6detail36g_pLookupGlobalManualFunctionPointerE");
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            lookup_global_t *symbol_val =
                (lookup_global_t *)(module->module_base + symbol->st_value);
            *symbol_val = g_LookupGlobalManualFunctionPointer;
        }
    }

    for (module_object_t *module = g_pAutoLoadList.back;
         module != (module_object_t *)&g_pAutoLoadList; module = module->prev) {
        resolve_symbols(module, true);
    }
}