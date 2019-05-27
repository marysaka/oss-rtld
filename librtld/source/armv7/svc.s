// Some required SVC
.align 4

.macro SVC_BEGIN name
    .section .text.\name, "ax", %progbits
    .global \name
    .type \name, %function
    .align 2
    .cfi_startproc
\name:
.endm

.macro SVC_END
    .cfi_endproc
.endm

SVC_BEGIN _ZN4rtld3svc11QueryMemoryEPNS0_10MemoryInfoEPjj
    str r1, [sp, #-4]!
    svc 0x6
    ldr r2, [sp]
    str r1, [r2]
    add sp, sp, #4
    bx lr
SVC_END

SVC_BEGIN _ZN4rtld3svc11ExitProcessEv
    svc 0x7
    bx lr
SVC_END

SVC_BEGIN _ZN4rtld3svc5BreakEjjj
    svc 0x26
    bx lr
SVC_END

SVC_BEGIN _ZN4rtld3svc17OutputDebugStringEPKcj
    svc 0x27
    bx lr
SVC_END

SVC_BEGIN _ZN4rtld3svc19ReturnFromExceptionEj
    svc 0x28
    bx lr
SVC_END
