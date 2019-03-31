#include "rtld.hpp"

// On the real rtld, those are calling musl functions that are stubbed.
extern "C" void __rtld_init(void) {}

extern "C" void __rtld_fini(void) {}