.section __TEXT,__text
.align 8

.globl _main

_main:
    pushq %rbp
    movq %rsp, %rbp
    callq _NSApplicationMain
    popq %rbp
    ret
