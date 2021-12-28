.global __module_start
.global __module_header

// Ensure we statically link against those.
.hidden __rtld_clean_bss
.hidden __rtld_relocate_self
.hidden main
.hidden SELF_MODULE_RUNTIME

.macro FUNC_RELATIVE_ASLR name, register_num, symbol
.word \symbol - .
\name:
    ldr w\register_num, [x30]
    sxtw x\register_num, w\register_num
    add x\register_num, x\register_num, x30
.endm

.section ".text.crt0","ax",%progbits

__module_start:
    b __rtld_shim_entrypoint
    .word __module_header - __module_start

__module_header:
    .ascii "MOD0"
    .word  __dynamic_start__      - __module_header
    .word  __bss_start__          - __module_header
    .word  __bss_end__            - __module_header
    .word  __eh_frame_hdr_start__ - __module_header
    .word  __eh_frame_hdr_end__   - __module_header
    .word  SELF_MODULE_RUNTIME    - __module_header

__rtld_shim_entrypoint:
    // Check if we entered because of an exception
    cmp x0, #0
    bne __rtld_entry_exception_shim
    // Preserve the thread handle
    mov w19, w1
    bl __rtld_entry_normal_shim
    b .

__rtld_entry_exception_shim:
    # TODO: finish this
    mov w0, 0xF801
    svc 0x28
    b .
__rtld_entry_normal_shim:
    // Clean .bss
    adrp x0, __bss_start__
    add x0, x0, #:lo12:__bss_start__
    adrp x1, __bss_end__
    add x1, x1, #:lo12:__bss_end__
    bl __rtld_clean_bss

    // Relocate current module
    adrp x0, __module_start
    add x0, x0, #:lo12:__module_start
    bl __rtld_relocate_self

    // Restore the thread handle
    adrp x0, __module_start
    add x0, x0, #:lo12:__module_start
    mov w1, w19
    // Now jump to main
    bl main
    b .