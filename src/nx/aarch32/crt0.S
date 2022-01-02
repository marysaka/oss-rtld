.global __module_start
.global __module_header

// Ensure we statically link against those.
.hidden __rtld_clean_bss
.hidden __rtld_relocate_self
.hidden main
.hidden __rtld_handle_exception
.hidden SELF_MODULE_RUNTIME
.hidden EXCEPTION_HANDLER_READY

.macro FUNC_RELATIVE_ASLR name, register_num, symbol
.word \symbol - .
\name:
    ldr r\register_num, [lr]
    add r\register_num, r\register_num, lr
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
    cmp r0, #0
    bne __rtld_entry_exception_shim
    // Preserve the thread handle
    mov r4, r1
    bl __rtld_entry_normal_shim
    b .

__rtld_entry_exception_shim:
    bl __rtld_entry_exception_shim1

FUNC_RELATIVE_ASLR __rtld_entry_exception_shim1, 2, EXCEPTION_HANDLER_READY
    ldr r2, [r2]
    cmp r2, 0
    beq __rtld_unhandled_exception
    bl __rtld_handle_exception

__rtld_unhandled_exception:
    mov r0, 0xF801
    svc 0x28
    b .

__rtld_entry_normal_shim:
    bl __rtld_entry_normal_shim1

FUNC_RELATIVE_ASLR __rtld_entry_normal_shim1, 0, __bss_start__
    bl __rtld_entry_normal_shim2

FUNC_RELATIVE_ASLR __rtld_entry_normal_shim2, 1, __bss_end__
    // Clean .bss
    bl __rtld_clean_bss
    bl __rtld_entry_normal_shim3

FUNC_RELATIVE_ASLR __rtld_entry_normal_shim3, 0, __module_start
    // Save module start to avoid recomputing it.
    mov r5, r0
    // Relocate current module
    bl __rtld_relocate_self

    // Restore module start
    mov r0, r5
    // Restore the thread handle
    mov r1, r4
    // Now jump to main
    bl main
    b .