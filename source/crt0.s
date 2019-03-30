.global __module_start
.global __rtld_runtime_resolve
.extern handle_exception
.extern relocate_self

.extern unknown_func
.extern modules_constructor_init
.extern rtld_lazy_bind_symbol


.weak   _ZN2nn4init5StartEmmPFvvES2_
.global _ZN2nn4init5StartEmmPFvvES2_

.macro FUNC_RELATIVE_ASLR name, register_num, symbol
.word \symbol - .
\name:
    ldr w\register_num, [x30]
    sxtw x\register_num, w\register_num
    add x\register_num, x\register_num, x30
.endm

.section ".text.crt0","ax"

__module_start:
    b __shim_entrypoint
    .word __nx_mod0 - __module_start

__shim_entrypoint:
    cmp x0, #0
    bne __entry_exception_shim
    // preserve main thread handle
    mov w19, w1
    bl __entry_normal_shim

FUNC_RELATIVE_ASLR __entry_normal_shim, 0, __bss_start__
    bl __clean_bss_shim

FUNC_RELATIVE_ASLR __clean_bss_shim, 2, __bss_end__
    sub x2, x2, x0
    mov w1, #0
    bl memset
    bl __start_shim

FUNC_RELATIVE_ASLR __start_shim, 0, __module_start
    bl __start

FUNC_RELATIVE_ASLR __start, 1, _DYNAMIC
    bl relocate_modules
    mov w0, w19
    adrp x1, __argdata__
    add x1, x1, #:lo12:__argdata__
    bl general_init_shim

FUNC_RELATIVE_ASLR general_init_shim, 2, unknown_func
    bl general_init
FUNC_RELATIVE_ASLR general_init, 3, modules_constructor_init
    bl _ZN2nn4init5StartEmmPFvvES2_
hang:
    b hang

__entry_exception_shim:
    b handle_exception

.section ".text", "ax"

#define ip0 x16
#define ip1 x17

__rtld_runtime_resolve:
    /* AArch64 we get called with:
    ip0         &PLTGOT[2]
    ip1         temp(dl resolver entry point)
    [sp, #0]    &PLTGOT[n]

    What we need:
    x0 = calling module (ip0[-2])
    x1 = .rel.plt index ((ip1 - ip0 - 8) / 8)
    */
    ldr ip1, [sp]
    str x29, [sp]
    stp x8, x19, [sp, #-0x10]!
    stp x6, x7, [sp, #-0x10]!
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    stp q6, q7, [sp, #-0x20]!
    stp q4, q5, [sp, #-0x20]!
    stp q2, q3, [sp, #-0x20]!
    stp q0, q1, [sp, #-0x20]!
    mov x29, sp
    mov x19, ip1
    sub x1, ip1, ip0
    sub x1, x1, #8
    lsr x1, x1, #3
    ldur x0, [ip0, #-8]
    bl rtld_lazy_bind_symbol
    str x0, [x19]
    mov ip0, x0
    ldp q0, q1, [sp], #0x20
    ldp q2, q3, [sp], #0x20
    ldp q4, q5, [sp], #0x20
    ldp q6, q7, [sp], #0x20
    ldp x0, x1, [sp], #0x10
    ldp x2, x3, [sp], #0x10
    ldp x4, x5, [sp], #0x10
    ldp x6, x7, [sp], #0x10
    ldp x8, x19, [sp], #0x10
    ldp x29, x30, [sp], #0x10
    br ip0

.section ".rodata.application_name"
.word 0
.word 8
.ascii "oss-rtld"

.section ".rodata"
.global __nx_mod0
__nx_mod0:
    .ascii "MOD0"
    .word  _DYNAMIC             - __nx_mod0
    .word  __bss_start__        - __nx_mod0
    .word  __bss_end__          - __nx_mod0
    .word  __eh_frame_hdr_start - __nx_mod0
    .word  __eh_frame_hdr_end   - __nx_mod0
    .word __nx_module_runtime - __nx_mod0