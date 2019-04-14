.global __module_start

.extern __rtld_handle_exception
.extern __rtld_relocate_modules

.extern __rtld_notify_exception_handler_ready
.extern __rtld_modules_init
.extern __rtld_lazy_bind_symbol
.extern __rtld_start_app
.extern g_IsExceptionHandlerReady

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
    bl __rtld_relocate_modules
    mov w0, w19
    adrp x1, __argdata__
    add x1, x1, #:lo12:__argdata__
    bl general_init_shim

FUNC_RELATIVE_ASLR general_init_shim, 2, __rtld_notify_exception_handler_ready
    bl general_init
FUNC_RELATIVE_ASLR general_init, 3, __rtld_modules_init
    bl __rtld_start_app
    b .

__entry_exception_shim:
    b handle_exception_shim
FUNC_RELATIVE_ASLR handle_exception_shim 2 g_IsExceptionHandlerReady
    cbz w2, unhandled_exception
    bl __rtld_handle_exception
unhandled_exception:
    mov w0, 0xf801
    bl svcReturnFromException
    b .

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
    .word  0
    .word  0
    .word __nx_module_runtime - __nx_mod0