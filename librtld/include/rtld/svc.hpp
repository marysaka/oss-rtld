#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "types.hpp"

namespace rtld {
namespace svc {

struct MemoryInfo {
    uint64_t address;
    uint64_t size;
    uint32_t type;
    uint32_t attribute;
    uint32_t permission;
    uint32_t device_ref_count;
    uint32_t ipc_ref_count;
    uint32_t padding;
};

_Static_assert(sizeof(MemoryInfo) == 0x28, "MemoryInfo size isn't valid");

Result QueryMemory(MemoryInfo *memory_info_ptr, uint32_t *pageinfo,
                   uintptr_t addr);

void ExitProcess(void);

Result Break(uint32_t breakReason, size_t unk, size_t info);

void ReturnFromException(Result errorCode);

Result OutputDebugString(const char *str, size_t str_size);
}  // namespace svc

}  // namespace rtld
