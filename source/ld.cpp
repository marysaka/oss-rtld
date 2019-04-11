#include "rtld.hpp"

Elf64_Addr lookup_global_auto(const char *name) {
    if (g_pAutoLoadList.back == (ModuleObject *)&g_pAutoLoadList) {
        return 0;
    }

    for (ModuleObject *module = g_pAutoLoadList.back;
         module != (ModuleObject *)&g_pAutoLoadList; module = module->prev) {
        Elf64_Sym *symbol = module->GetSymbolByName(name);
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            return module->module_base + symbol->st_value;
        }
    }
    return 0;
}

static inline void resolve_symbol_rel_absolute(ModuleObject *module_object,
                                               Elf64_Rel *entry) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_ABS32 || r_type == R_AARCH64_ABS64 ||
        r_type == R_AARCH64_GLOB_DAT) {
        Elf64_Sym *symbol = &module_object->dynsym[r_sym];
        Elf64_Addr target_symbol_address;

        if (module_object->TryResolveSymbol(&target_symbol_address, symbol)) {
            uint64_t *target =
                (uint64_t *)(module_object->module_base + entry->r_offset);
            *target += target_symbol_address;
        } else if (g_RoDebugFlag) {
            print_unresolved_symbol(&module_object->dynstr[symbol->st_name]);
        }
    }
}

static inline void resolve_symbol_rela_absolute(ModuleObject *module_object,
                                                Elf64_Rela *entry) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_ABS32 || r_type == R_AARCH64_ABS64 ||
        r_type == R_AARCH64_GLOB_DAT) {
        Elf64_Sym *symbol = &module_object->dynsym[r_sym];
        Elf64_Addr target_symbol_address;

        if (module_object->TryResolveSymbol(&target_symbol_address, symbol)) {
            uint64_t *target =
                (uint64_t *)(module_object->module_base + entry->r_offset);
            *target = target_symbol_address + entry->r_addend;
        } else if (g_RoDebugFlag) {
            print_unresolved_symbol(&module_object->dynstr[symbol->st_name]);
        }
    }
}

static inline void resolve_symbol_rel_jump_slot(ModuleObject *module_object,
                                                Elf64_Rel *entry,
                                                bool do_lazy_got_init) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_JUMP_SLOT) {
        uint64_t *target =
            (uint64_t *)(module_object->module_base + entry->r_offset);
        uint64_t target_address = module_object->module_base + *target;
        if (do_lazy_got_init) {
            *target = target_address;
        }

        if (module_object->got_stub_ptr) {
            if (module_object->got_stub_ptr != target_address) {
                svcBreak(0, 0, 0);
            }
        } else {
            module_object->got_stub_ptr = target_address;
        }

        // We are in the non lazy case
        if (!do_lazy_got_init) {
            Elf64_Sym *symbol = &module_object->dynsym[r_sym];
            Elf64_Addr target_symbol_address;

            if (module_object->TryResolveSymbol(&target_symbol_address,
                                                symbol)) {
                *target += target_symbol_address;
            } else {
                if (g_RoDebugFlag) {
                    print_unresolved_symbol(
                        &module_object->dynstr[symbol->st_name]);
                }
                *target = target_address;
            }
        }
    }
}

static inline void resolve_symbol_rela_jump_slot(ModuleObject *module_object,
                                                 Elf64_Rela *entry,
                                                 bool do_lazy_got_init) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_JUMP_SLOT) {
        uint64_t *target =
            (uint64_t *)(module_object->module_base + entry->r_offset);
        uint64_t target_address = module_object->module_base + *target;
        if (do_lazy_got_init) {
            *target = target_address;
        }

        if (module_object->got_stub_ptr) {
            if (module_object->got_stub_ptr != target_address) {
                svcBreak(0, 0, 0);
            }
        } else {
            module_object->got_stub_ptr = target_address;
        }

        // We are in the non lazy case
        if (!do_lazy_got_init) {
            Elf64_Sym *symbol = &module_object->dynsym[r_sym];
            Elf64_Addr target_symbol_address;

            if (module_object->TryResolveSymbol(&target_symbol_address,
                                                symbol)) {
                *target = target_symbol_address + entry->r_addend;
            } else {
                if (g_RoDebugFlag) {
                    print_unresolved_symbol(
                        &module_object->dynstr[symbol->st_name]);
                }
                *target = target_address;
            }
        }
    }
}

extern "C" void __rtld_runtime_resolve(void);

extern "C" Elf64_Addr __rtld_lazy_bind_symbol(ModuleObject *module,
                                              uint64_t index) {
    if (module->is_rela) {
        Elf64_Rela *entry = &module->rela_or_rel_plt.rela[index];
        Elf64_Sym *symbol = &module->dynsym[ELF64_R_SYM(entry->r_info)];

        Elf64_Addr target_symbol_address;

        if (module->TryResolveSymbol(&target_symbol_address, symbol)) {
            if (target_symbol_address == 0) {
                return 0;
            }

            return target_symbol_address + entry->r_addend;
        } else {
            print_unresolved_symbol(&module->dynstr[symbol->st_name]);
        }
    } else {
        Elf64_Rel *entry = &module->rela_or_rel_plt.rel[index];
        Elf64_Sym *symbol = &module->dynsym[ELF64_R_SYM(entry->r_info)];

        Elf64_Addr target_symbol_address;

        if (module->TryResolveSymbol(&target_symbol_address, symbol)) {
            return target_symbol_address;
        } else {
            print_unresolved_symbol(&module->dynstr[symbol->st_name]);
        }
    }

    return 0;
}

extern "C" void __rtld_modules_init(void) {
    for (ModuleObject *module = g_pAutoLoadList.front;
         module != (ModuleObject *)&g_pAutoLoadList; module = module->next) {
        module->dt_init();
    }
}

void resolve_symbols(ModuleObject *module_object, bool do_lazy_got_init) {
    for (uint64_t index = module_object->rel_count;
         index < module_object->rel_dyn_size / sizeof(Elf64_Rel); index++) {
        Elf64_Rel *entry = &module_object->rela_or_rel.rel[index];
        resolve_symbol_rel_absolute(module_object, entry);
    }

    for (uint64_t index = module_object->rela_count;
         index < module_object->rela_dyn_size / sizeof(Elf64_Rela); index++) {
        Elf64_Rela *entry = &module_object->rela_or_rel.rela[index];
        resolve_symbol_rela_absolute(module_object, entry);
    }

    if (module_object->is_rela) {
        if (module_object->rela_or_rel_plt_size >= sizeof(Elf64_Rela)) {
            for (uint64_t index = 0;
                 index <
                 module_object->rela_or_rel_plt_size / sizeof(Elf64_Rela);
                 index++) {
                Elf64_Rela *entry = &module_object->rela_or_rel_plt.rela[index];
                resolve_symbol_rela_jump_slot(module_object, entry,
                                              do_lazy_got_init);
            }
        }
    } else if (module_object->rela_or_rel_plt_size >= sizeof(Elf64_Rel)) {
        for (uint64_t index = 0;
             index < module_object->rela_or_rel_plt_size / sizeof(Elf64_Rel);
             index++) {
            Elf64_Rel *entry = &module_object->rela_or_rel_plt.rel[index];
            resolve_symbol_rel_jump_slot(module_object, entry,
                                         do_lazy_got_init);
        }
    }

    if (module_object->got) {
        module_object->got[1] = module_object;
        module_object->got[2] = (void *)__rtld_runtime_resolve;
    }
}