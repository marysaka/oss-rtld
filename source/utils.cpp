#include "rtld.hpp"

uint64_t elf_hash(const char *name) {
    uint64_t h = 0;
    uint64_t g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}