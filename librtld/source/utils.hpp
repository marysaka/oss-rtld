#pragma once

#include <rtld.hpp>

#define WEAK __attribute__((weak))

extern "C" WEAK void *memset(void *s, int c, size_t n);
extern "C" size_t __rtld_strlen(const char *s);
extern "C" int __rtld_strcmp(const char *s1, const char *s2);
extern "C" unsigned long __rtld_elf_hash(const char *name);

inline void debug_print(const char *str) {
    svc::OutputDebugString(str, __rtld_strlen(str));
}

inline void print_unresolved_symbol(const char *name) {
    debug_print("[rtld] warning: unresolved symbol = '");
    debug_print(name);
    debug_print("'\n");
}
