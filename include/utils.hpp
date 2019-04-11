#pragma once

#include <cstring>

#include "svc.h"

#define WEAK __attribute__((weak))

inline void debug_print(const char *str) {
    svcOutputDebugString(str, strlen(str));
}

static inline void print_unresolved_symbol(const char *name) {
    debug_print("[rtld] warning: unresolved symbol = '");
    debug_print(name);
    debug_print("'\n");
}

uint64_t elf_hash(const char *name);