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

SVC_BEGIN svcQueryMemory
    str x1, [sp, #-16]!
    svc 0x6
    ldr x2, [sp], #16
    str w1, [x2]
    ret
SVC_END

SVC_BEGIN svcBreak
    svc 0x26
    ret
SVC_END

SVC_BEGIN svcOutputDebugString
    svc 0x27
    ret
SVC_END

SVC_BEGIN svcReturnFromException
    svc 0x28
    ret
SVC_END

