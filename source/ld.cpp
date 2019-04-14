#include "rtld.hpp"

Elf64_Addr lookup_global_auto(const char *name) {
    if (g_pAutoLoadList.back == (ModuleObject *)&g_pAutoLoadList) {
        return 0;
    }

    for (ModuleObject *module : g_pAutoLoadList) {
        Elf64_Sym *symbol = module->GetSymbolByName(name);
        if (symbol && ELF64_ST_BIND(symbol->st_info)) {
            return module->module_base + symbol->st_value;
        }
    }
    return 0;
}

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
    for (auto iter = g_pAutoLoadList.rbegin(); iter != g_pAutoLoadList.rend();
         ++iter) {
        auto module = *iter;
        module->dt_init();
    }
}
