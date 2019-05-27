.global __module_start

.extern __rtld_handle_exception
.extern __rtld_relocate_modules

.extern __rtld_notify_exception_handler_ready
.extern __rtld_modules_init
.extern __rtld_lazy_bind_symbol
.extern __rtld_start_app

.extern _ZN4rtld19__nx_module_runtimeE
.extern _ZN4rtld25g_IsExceptionHandlerReadyE
.extern _ZN4rtld3svc19ReturnFromExceptionEm
.extern __nx_mod0

.macro FUNC_RELATIVE_ASLR name, register_num, symbol
.word \symbol - .
\name:
    ldr w\register_num, [x30]
    sxtw x\register_num, w\register_num
    add x\register_num, x\register_num, x30
.endm

.section ".text.crt0","ax"

__module_start:
    b __rtld_shim_entrypoint
    .word __nx_mod0 - __module_start

__rtld_shim_entrypoint:
    cmp x0, #0
    bne __rtld_entry_exception_shim
    // preserve main thread handle
    mov w19, w1
    bl __rtld_entry_normal_shim

FUNC_RELATIVE_ASLR __rtld_entry_normal_shim, 0, __bss_start__
    bl __rtld_clean_bss_shim

FUNC_RELATIVE_ASLR __rtld_clean_bss_shim, 2, __bss_end__
    sub x2, x2, x0
    mov w1, #0
    bl memset
    bl __rtld_start_shim

FUNC_RELATIVE_ASLR __rtld_start_shim, 0, __module_start
    bl __rtld_start

FUNC_RELATIVE_ASLR __rtld_start, 1, __dynamic_start__
    bl __rtld_relocate_modules
    mov w0, w19
    adrp x1, __argdata__
    add x1, x1, #:lo12:__argdata__
    bl __rtld_general_init_shim

FUNC_RELATIVE_ASLR __rtld_general_init_shim, 2, __rtld_notify_exception_handler_ready
    bl __rtld_general_init
FUNC_RELATIVE_ASLR __rtld_general_init, 3, __rtld_modules_init
    bl __rtld_start_app
    b .

__rtld_entry_exception_shim:
    b __rtld_handle_exception_shim
FUNC_RELATIVE_ASLR __rtld_handle_exception_shim 2 _ZN4rtld25g_IsExceptionHandlerReadyE
    cbz w2, __rtld_unhandled_exception
    bl __rtld_handle_exception
__rtld_unhandled_exception:
    mov w0, 0xf801
    bl _ZN4rtld3svc19ReturnFromExceptionEm
    b .
