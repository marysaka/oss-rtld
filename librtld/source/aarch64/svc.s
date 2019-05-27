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

SVC_BEGIN _ZN4rtld3svc11QueryMemoryEPNS0_10MemoryInfoEPjm
    str x1, [sp, #-16]!
    svc 0x6
    ldr x2, [sp], #16
    str w1, [x2]
    ret
SVC_END

SVC_BEGIN _ZN4rtld3svc11ExitProcessEv
    svc 0x7
    ret
SVC_END

SVC_BEGIN _ZN4rtld3svc5BreakEjmm
    svc 0x26
    ret
SVC_END

SVC_BEGIN _ZN4rtld3svc17OutputDebugStringEPKcm
    svc 0x27
    ret
SVC_END

SVC_BEGIN _ZN4rtld3svc19ReturnFromExceptionEm
    svc 0x28
    ret
SVC_END

