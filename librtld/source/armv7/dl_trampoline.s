.section ".text", "ax"

.global __rtld_runtime_resolve
.type __rtld_runtime_resolve, %function
__rtld_runtime_resolve:
    /* ARMv7 we get called with:
    stack[0] contains the return address from this call
    ip contains &GOT[n+3] (pointer to function)
    lr points to &GOT[2]

    lr                &GOT[2]
    ip (r12)          &GOT[n]

    What we need:
    r0 = calling module (lr[-1])
    r1 = .rel.plt index ((ip - lr - 4) / 4)
    */
    stmfd sp!, {r0-r4}
    vpush {d0-d7}
    sub r1, r12, lr
    sub r1, r1, 4
    mov r1, r1, lsr #2
    ldr r0, [lr, #-4]
    mov r4, r12
    bl __rtld_lazy_bind_symbol
    str r0, [r4]
    mov r12, r0
    vpop {d0-d7}
    ldmfd sp!, {r0-r4, lr}
    bx r12
