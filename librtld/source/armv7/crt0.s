.global __module_start

.extern __rtld_handle_exception
.extern __rtld_relocate_modules

.extern __rtld_notify_exception_handler_ready
.extern __rtld_modules_init
.extern __rtld_lazy_bind_symbol
.extern __rtld_start_app

.extern _ZN4rtld19__nx_module_runtimeE
.extern _ZN4rtld25g_IsExceptionHandlerReadyE
.extern _ZN4rtld3svc19ReturnFromExceptionEj
.extern __nx_mod0

.macro FUNC_RELATIVE_ASLR name, register_num, symbol
.word \symbol - .
\name:
    ldr r\register_num, [lr]
    add r\register_num, r\register_num, lr
.endm

.section ".text.crt0","ax"

__module_start:
    b __rtld_shim_entrypoint
    .word __nx_mod0 - __module_start

__rtld_shim_entrypoint:
    cmp r0, #0
    bne __rtld_entry_exception_shim
    // preserve main thread handle
    mov r4, r1
    bl __rtld_entry_normal_shim

FUNC_RELATIVE_ASLR __rtld_entry_normal_shim, 0, __bss_start__
    bl __rtld_clean_bss_shim

FUNC_RELATIVE_ASLR __rtld_clean_bss_shim, 2, __bss_end__
    sub r2, r2, r0
    mov r1, #0
    bl memset
    bl __rtld_start_shim

FUNC_RELATIVE_ASLR __rtld_start_shim, 0, __module_start
    bl __rtld_start

FUNC_RELATIVE_ASLR __rtld_start, 1, __dynamic_start__
    bl __rtld_relocate_modules
    mov r0, r4
    bl __rtld_prepare_general_init

# Nintendo use a relocation for the __argdata__ but I don't want to,
# so here is another FUNC_RELATIVE_ASLR
FUNC_RELATIVE_ASLR __rtld_prepare_general_init, 1, __argdata__
    bl __rtld_general_init_shim
FUNC_RELATIVE_ASLR __rtld_general_init_shim, 2, __rtld_notify_exception_handler_ready
    bl __rtld_general_init
FUNC_RELATIVE_ASLR __rtld_general_init, 3, __rtld_modules_init
    bl __rtld_start_app
    b .

__rtld_entry_exception_shim:
    b __rtld_handle_exception_shim
FUNC_RELATIVE_ASLR __rtld_handle_exception_shim 2 _ZN4rtld25g_IsExceptionHandlerReadyE
    cmp r2, 0
    beq __rtld_unhandled_exception
    bl __rtld_handle_exception
__rtld_unhandled_exception:
    ldr r0, =0xf801
    bl _ZN4rtld3svc19ReturnFromExceptionEj
    b .