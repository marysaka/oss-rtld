#include "utils.hpp"
#include "rtld.hpp"

// TODO: provide better implmentations of those functions.
extern "C" WEAK void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

extern "C" size_t __rtld_strlen(const char *s) {
    size_t res = 0;

    while (*s) {
        res++;
        s++;
    }

    return res;
}

extern "C" int __rtld_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (*(const unsigned char *)s1 - *(const unsigned char *)s2);
}

extern "C" unsigned long __rtld_elf_hash(const char *name) {
    unsigned long h = 0;
    unsigned long g;

    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}
