#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MOD0_MAGIC 0x30444F4D

typedef struct {
    uint32_t magic;
    uint32_t dynamic_offset;
    uint32_t bss_start_offset;
    uint32_t bss_end_offset;
    uint32_t unwind_start_offset;
    uint32_t unwind_end_offset;
    uint32_t module_object_offset;
} module_header_t;

#ifdef __cplusplus
}
#endif