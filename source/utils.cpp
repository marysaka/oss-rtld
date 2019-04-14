#include "rtld.hpp"

// TODO: provide better implmentations of those functions.
extern "C" void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

extern "C" size_t strlen(const char *s) {
    size_t res = 0;

    while (*s) {
        res++;
        s++;
    }

    return res;
}

extern "C" int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*(const unsigned char *)s1 - *(const unsigned char *)s2);
}

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
