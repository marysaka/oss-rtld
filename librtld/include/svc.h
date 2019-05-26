#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef size_t Result;
typedef uint32_t Handle;

typedef struct {
    uint64_t address;
    uint64_t size;
    uint32_t type;
    uint32_t attribute;
    uint32_t permission;
    uint32_t device_ref_count;
    uint32_t ipc_ref_count;
    uint32_t padding;
} memory_info_t;

_Static_assert(sizeof(memory_info_t) == 0x28, "memory_info_t size isn't valid");

Result svcQueryMemory(memory_info_t *memory_info_ptr, uint32_t *pageinfo,
                      uintptr_t addr);

void svcExitProcess(void);

Result svcBreak(uint32_t breakReason, size_t unk, size_t info);

void svcReturnFromException(Result errorCode);

Result svcOutputDebugString(const char *str, size_t str_size);

#ifdef __cplusplus
}
#endif