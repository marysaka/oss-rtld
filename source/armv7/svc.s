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
    str r1, [sp, #-4]!
    svc 0x6
    ldr r2, [sp]
    str r1, [r2]
    add sp, sp, #4
    bx lr
SVC_END

SVC_BEGIN svcExitProcess
    svc 0x7
    bx lr
SVC_END

SVC_BEGIN svcBreak
    svc 0x26
    bx lr
SVC_END

SVC_BEGIN svcOutputDebugString
    svc 0x27
    bx lr
SVC_END

SVC_BEGIN svcReturnFromException
    svc 0x28
    bx lr
SVC_END
