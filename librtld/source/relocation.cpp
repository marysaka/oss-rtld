#include "rtld.hpp"

namespace rtld {
ModuleObjectList g_pManualLoadList;
ModuleObjectList g_pAutoLoadList;
bool g_RoDebugFlag;
lookup_global_t g_LookupGlobalManualFunctionPointer;
__attribute__((section(".bss"))) ModuleObject __nx_module_runtime;
};  // namespace rtld

inline void relocate_self(char *alsr_base, Elf_Dyn *dynamic) {
    Elf_Addr rela = 0;
    Elf_Addr rel = 0;

    Elf_Xword rela_entry_size = sizeof(Elf_Rela);
    Elf_Xword rel_entry_size = sizeof(Elf_Rel);

    Elf_Xword rela_entry_count = 0;
    Elf_Xword rel_entry_count = 0;

    for (; dynamic->d_tag != DT_NULL; dynamic++) {
        switch (dynamic->d_tag) {
            case DT_RELA:
                rela = ((Elf_Addr)alsr_base + dynamic->d_un.d_ptr);
                break;

            case DT_RELAENT:
                rela_entry_size = dynamic->d_un.d_val;
                break;

            case DT_REL:
                rel = ((Elf_Addr)alsr_base + dynamic->d_un.d_ptr);
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
        Elf_Xword i = 0;

        while (i < rel_entry_count) {
            Elf_Rel *entry = (Elf_Rel *)(rel + (i * rel_entry_size));
            switch (ELF_R_TYPE(entry->r_info)) {
                case ARCH_RELATIVE: {
                    Elf_Addr *ptr = (Elf_Addr *)(alsr_base + entry->r_offset);
                    *ptr += (Elf_Addr)alsr_base;
                    break;
                }
            }
            i++;
        }
    }

    if (rela_entry_count) {
        Elf_Xword i = 0;

        while (i < rela_entry_count) {
            Elf_Rela *entry = (Elf_Rela *)(rela + (i * rela_entry_size));
            switch (ELF_R_TYPE(entry->r_info)) {
                case ARCH_RELATIVE: {
                    Elf_Addr *ptr = (Elf_Addr *)(alsr_base + entry->r_offset);
                    *ptr = (Elf_Addr)alsr_base + entry->r_addend;
                    break;
                }
            }
            i++;
        }
    }
}

extern "C" void __rtld_relocate_modules(char *aslr_base, Elf_Dyn *dynamic) {
    relocate_self(aslr_base, dynamic);

    // Init global lists
    // Those are only used to know the end and the start of the list
    g_pManualLoadList.front = (ModuleObject *)&g_pManualLoadList;
    g_pManualLoadList.back = (ModuleObject *)&g_pManualLoadList;

    g_pAutoLoadList.front = (ModuleObject *)&g_pAutoLoadList;
    g_pAutoLoadList.back = (ModuleObject *)&g_pAutoLoadList;

    __nx_module_runtime.prev = &__nx_module_runtime;
    __nx_module_runtime.next = &__nx_module_runtime;

    __nx_module_runtime.Initialize(aslr_base, dynamic);

    ModuleObject *next_module = __nx_module_runtime.next;
    ModuleObject *front_module = g_pAutoLoadList.front;

    __nx_module_runtime.next = front_module;
    next_module->prev = (ModuleObject *)&g_pAutoLoadList;
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
            memory_info.address != (uint64_t)aslr_base) {
            char *module_header_address =
                (char *)(memory_info.address +
                         *(uint32_t *)(memory_info.address + 4));
            ModuleHeader *module_header = (ModuleHeader *)module_header_address;
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

            ModuleObject *module_object =
                (ModuleObject *)(module_header_address +
                                 module_header->module_object_offset);
            Elf_Dyn *module_dynamic =
                (Elf_Dyn *)(module_header_address +
                            module_header->dynamic_offset);

            module_object->next = module_object;
            module_object->prev = module_object;
            module_object->Initialize((char *)memory_info.address,
                                      module_dynamic);
            module_object->Relocate();

            next_module = module_object->next;
            module_object->next = g_pAutoLoadList.front;
            next_module->prev = (ModuleObject *)&g_pAutoLoadList;
            g_pAutoLoadList.front->prev = module_object;
            g_pAutoLoadList.front = next_module;
        }

        is_past_last_address =
            memory_info.address + memory_info.size > last_address;
        last_address = memory_info.address + memory_info.size;
    } while (is_past_last_address);

    if (g_pAutoLoadList.back == (ModuleObject *)&g_pAutoLoadList) {
        return;
    }

    for (ModuleObject *module : g_pAutoLoadList) {
        Elf_Sym *symbol =
            module->GetSymbolByName("_ZN2nn2ro6detail15g_pAutoLoadListE");
        if (symbol && ELF_ST_BIND(symbol->st_info)) {
            ModuleObjectList **symbol_val =
                (ModuleObjectList **)(module->module_base + symbol->st_value);
            *symbol_val = &g_pAutoLoadList;
        }

        symbol =
            module->GetSymbolByName("_ZN2nn2ro6detail17g_pManualLoadListE");
        if (symbol && ELF_ST_BIND(symbol->st_info)) {
            ModuleObjectList **symbol_val =
                (ModuleObjectList **)(module->module_base + symbol->st_value);
            *symbol_val = &g_pManualLoadList;
        }

        symbol = module->GetSymbolByName("_ZN2nn2ro6detail14g_pRoDebugFlagE");
        if (symbol && ELF_ST_BIND(symbol->st_info)) {
            bool **symbol_val =
                (bool **)(module->module_base + symbol->st_value);
            *symbol_val = &g_RoDebugFlag;
        }

        symbol = module->GetSymbolByName(
            "_ZN2nn2ro6detail34g_pLookupGlobalAutoFunctionPointerE");
        if (symbol && ELF_ST_BIND(symbol->st_info)) {
            lookup_global_t *symbol_val =
                (lookup_global_t *)(module->module_base + symbol->st_value);
            *symbol_val = lookup_global_auto;
        }

        symbol = module->GetSymbolByName(
            "_ZN2nn2ro6detail36g_pLookupGlobalManualFunctionPointerE");
        if (symbol && ELF_ST_BIND(symbol->st_info)) {
            lookup_global_t *symbol_val =
                (lookup_global_t *)(module->module_base + symbol->st_value);
            *symbol_val = g_LookupGlobalManualFunctionPointer;
        }
    }

    for (ModuleObject *module : g_pAutoLoadList) {
        module->ResolveSymbols(true);
    }
}