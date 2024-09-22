.section __TEXT,__text
.align 8

.globl _main

#ifdef __x86_64__

_main:
    pushq %rbp
    movq %rsp, %rbp
    callq _NSApplicationMain
    popq %rbp
    ret

#elif defined(__arm64__)

_main:
    sub    sp, sp, #16
    stp    x29, x30, [sp]
    add    x29, sp, #16
    bl    _NSApplicationMain
    ldp    x29, x30, [sp]
    add    sp, sp, #16
    ret

#else /* Unknown arch! */

#error "Unsupported Architecture!"

#endif /* archs */
