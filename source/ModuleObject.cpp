#include "rtld.hpp"

namespace rtld {

void ModuleObject::Initialize(uint64_t aslr_base, Elf64_Dyn *dynamic) {
#ifdef __RTLD_6XX__
    this->nro_size = 0;
    this->soname_idx = 0;
    this->cannot_revert_symbols = false;
#endif

    this->is_rela = false;
    this->module_base = aslr_base;
    this->dyanmic = dynamic;
    this->rela_or_rel_plt_size = 0;
    this->dt_init = nullptr;
    this->dt_fini = nullptr;
    this->hash_bucket = nullptr;
    this->hash_chain = nullptr;
    this->dynstr = nullptr;
    this->dynsym = nullptr;
    this->dynstr_size = 0;
    this->rela_or_rel.rela = nullptr;
    this->got = nullptr;
    this->rela_dyn_size = 0;
    this->rel_dyn_size = 0;
    this->got_stub_ptr = 0;

    void *rel_plt = nullptr;

    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        switch (dynamic->d_tag) {
            case DT_PLTRELSZ: {
                this->rela_or_rel_plt_size = dynamic->d_un.d_val;
                break;
            }

            case DT_PLTGOT: {
                this->got = (void **)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_HASH: {
                uint32_t *hash_table =
                    (uint32_t *)(aslr_base + dynamic->d_un.d_val);

                const uint32_t nbucket = hash_table[0];
                const uint32_t nchain = hash_table[1];

                this->hash_nbucket_value = nbucket;
                this->hash_nchain_value = nchain;
                this->hash_bucket = &hash_table[2];
                this->hash_chain = &hash_table[2 + nbucket];
                break;
            }

            case DT_STRTAB: {
                this->dynstr = (char *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_SYMTAB: {
                this->dynsym = (Elf64_Sym *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_REL: {
                this->rela_or_rel.rel =
                    (Elf64_Rel *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELA: {
                this->rela_or_rel.rela =
                    (Elf64_Rela *)(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELASZ: {
                this->rela_dyn_size = dynamic->d_un.d_val;
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
                this->dynstr_size = dynamic->d_un.d_val;
                break;
            }

            case DT_INIT: {
                this->dt_init = (void (*)())(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_FINI: {
                this->dt_fini = (void (*)())(aslr_base + dynamic->d_un.d_val);
                break;
            }

            case DT_RELSZ: {
                this->rel_dyn_size = dynamic->d_un.d_val;
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
                this->is_rela = value == DT_RELA;
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
                this->rela_count = dynamic->d_un.d_val;
                break;

            case DT_RELCOUNT:
                this->rel_count = dynamic->d_un.d_val;
                break;

            case DT_SONAME:
#ifdef __RTLD_6XX__
                this->soname_idx = dynamic->d_un.d_val;
#endif
                break;

            case DT_NEEDED:
            case DT_RPATH:
            case DT_SYMBOLIC:
            case DT_DEBUG:
            case DT_TEXTREL:
            default:
                break;
        }
    }

    this->rela_or_rel_plt.raw = rel_plt;
}

void ModuleObject::Relocate() {
    if (this->rela_count) {
        for (uint64_t i = 0; i < this->rel_count; i++) {
            Elf64_Rel *entry = &this->rela_or_rel.rel[i];
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr =
                        (uint64_t *)(this->module_base + entry->r_offset);
                    *ptr += this->module_base;
                    break;
                }
            }
        }
    }

    if (this->rela_count) {
        for (uint64_t i = 0; i < this->rela_count; i++) {
            Elf64_Rela *entry = &this->rela_or_rel.rela[i];
            switch (ELF64_R_TYPE(entry->r_info)) {
                case R_AARCH64_RELATIVE: {
                    uint64_t *ptr =
                        (uint64_t *)(this->module_base + entry->r_offset);
                    *ptr = this->module_base + entry->r_addend;
                    break;
                }
            }
        }
    }
}

Elf64_Sym *ModuleObject::GetSymbolByName(const char *name) {
    uint64_t name_hash = elf_hash(name);

    for (uint32_t i = this->hash_bucket[name_hash % this->hash_nbucket_value];
         i; i = this->hash_chain[i]) {
        bool is_common = this->dynsym[i].st_shndx
                             ? this->dynsym[i].st_shndx == SHN_COMMON
                             : true;
        if (!is_common &&
            strcmp(name, this->dynstr + this->dynsym[i].st_name) == 0) {
            return &this->dynsym[i];
        }
    }

    return nullptr;
}

bool ModuleObject::TryResolveSymbol(Elf64_Addr *target_symbol_address,
                                    Elf64_Sym *symbol) {
    const char *name = &this->dynstr[symbol->st_name];

    if (ELF64_ST_VISIBILITY(symbol->st_other)) {
        Elf64_Sym *target_symbol = this->GetSymbolByName(name);
        if (target_symbol) {
            *target_symbol_address =
                this->module_base + target_symbol->st_value;
            return true;
        } else if ((ELF64_ST_BIND(symbol->st_info) & STB_WEAK) == STB_WEAK) {
            *target_symbol_address = 0;
            return true;
        }
    } else {
        Elf64_Addr address = lookup_global_auto(name);

        if (address == 0 && g_LookupGlobalManualFunctionPointer) {
            address = g_LookupGlobalManualFunctionPointer(name);
        }

        if (address != 0) {
            *target_symbol_address = address;
            return true;
        }
    }
    return false;
}

void ModuleObject::ResolveSymbolRelAbsolute(Elf64_Rel *entry) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_ABS32 || r_type == R_AARCH64_ABS64 ||
        r_type == R_AARCH64_GLOB_DAT) {
        Elf64_Sym *symbol = &this->dynsym[r_sym];
        Elf64_Addr target_symbol_address;

        if (this->TryResolveSymbol(&target_symbol_address, symbol)) {
            uint64_t *target =
                (uint64_t *)(this->module_base + entry->r_offset);
            *target += target_symbol_address;
        } else if (g_RoDebugFlag) {
            print_unresolved_symbol(&this->dynstr[symbol->st_name]);
        }
    }
}

void ModuleObject::ResolveSymbolRelaAbsolute(Elf64_Rela *entry) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_ABS32 || r_type == R_AARCH64_ABS64 ||
        r_type == R_AARCH64_GLOB_DAT) {
        Elf64_Sym *symbol = &this->dynsym[r_sym];
        Elf64_Addr target_symbol_address;

        if (this->TryResolveSymbol(&target_symbol_address, symbol)) {
            uint64_t *target =
                (uint64_t *)(this->module_base + entry->r_offset);
            *target = target_symbol_address + entry->r_addend;
        } else if (g_RoDebugFlag) {
            print_unresolved_symbol(&this->dynstr[symbol->st_name]);
        }
    }
}

void ModuleObject::ResolveSymbolRelJumpSlot(Elf64_Rel *entry,
                                            bool do_lazy_got_init) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_JUMP_SLOT) {
        uint64_t *target = (uint64_t *)(this->module_base + entry->r_offset);
        uint64_t target_address = this->module_base + *target;
        if (do_lazy_got_init) {
            *target = target_address;
        }

        if (this->got_stub_ptr) {
            if (this->got_stub_ptr != target_address) {
                svcBreak(0, 0, 0);
            }
        } else {
            this->got_stub_ptr = target_address;
        }

        // We are in the non lazy case
        if (!do_lazy_got_init) {
            Elf64_Sym *symbol = &this->dynsym[r_sym];
            Elf64_Addr target_symbol_address;

            if (this->TryResolveSymbol(&target_symbol_address, symbol)) {
                *target += target_symbol_address;
            } else {
                if (g_RoDebugFlag) {
                    print_unresolved_symbol(&this->dynstr[symbol->st_name]);
                }
                *target = target_address;
            }
        }
    }
}

void ModuleObject::ResolveSymbolRelaJumpSlot(Elf64_Rela *entry,
                                             bool do_lazy_got_init) {
    uint32_t r_type = ELF64_R_TYPE(entry->r_info);
    uint32_t r_sym = ELF64_R_SYM(entry->r_info);

    if (r_type == R_AARCH64_JUMP_SLOT) {
        uint64_t *target = (uint64_t *)(this->module_base + entry->r_offset);
        uint64_t target_address = this->module_base + *target;
        if (do_lazy_got_init) {
            *target = target_address;
        }

        if (this->got_stub_ptr) {
            if (this->got_stub_ptr != target_address) {
                svcBreak(0, 0, 0);
            }
        } else {
            this->got_stub_ptr = target_address;
        }

        // We are in the non lazy case
        if (!do_lazy_got_init) {
            Elf64_Sym *symbol = &this->dynsym[r_sym];
            Elf64_Addr target_symbol_address;

            if (this->TryResolveSymbol(&target_symbol_address, symbol)) {
                *target = target_symbol_address + entry->r_addend;
            } else {
                if (g_RoDebugFlag) {
                    print_unresolved_symbol(&this->dynstr[symbol->st_name]);
                }
                *target = target_address;
            }
        }
    }
}

void ModuleObject::ResolveSymbols(bool do_lazy_got_init) {
    for (uint64_t index = this->rel_count;
         index < this->rel_dyn_size / sizeof(Elf64_Rel); index++) {
        Elf64_Rel *entry = &this->rela_or_rel.rel[index];
        this->ResolveSymbolRelAbsolute(entry);
    }

    for (uint64_t index = this->rela_count;
         index < this->rela_dyn_size / sizeof(Elf64_Rela); index++) {
        Elf64_Rela *entry = &this->rela_or_rel.rela[index];
        this->ResolveSymbolRelaAbsolute(entry);
    }

    if (this->is_rela) {
        if (this->rela_or_rel_plt_size >= sizeof(Elf64_Rela)) {
            for (uint64_t index = 0;
                 index < this->rela_or_rel_plt_size / sizeof(Elf64_Rela);
                 index++) {
                Elf64_Rela *entry = &this->rela_or_rel_plt.rela[index];
                this->ResolveSymbolRelaJumpSlot(entry, do_lazy_got_init);
            }
        }
    } else if (this->rela_or_rel_plt_size >= sizeof(Elf64_Rel)) {
        for (uint64_t index = 0;
             index < this->rela_or_rel_plt_size / sizeof(Elf64_Rel); index++) {
            Elf64_Rel *entry = &this->rela_or_rel_plt.rel[index];
            this->ResolveSymbolRelJumpSlot(entry, do_lazy_got_init);
        }
    }

    if (this->got) {
        this->got[1] = this;
        this->got[2] = (void *)__rtld_runtime_resolve;
    }
}

}  // namespace rtld