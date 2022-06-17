// assembly-only factorial
// 2021-02-15

    .text
    .global _start
    .extern intwrite, newlinewrite

    .set n, 16

    .lcomm facbuffer, 65   // space for int2str conversions

_start:
    movz x11, 1         // x11: each value of n
outer:
    mov  x0, x11        // x10: current value of n
    bl   intwrite
    bl   blankwrite
    bl   blankwrite

    mov  x10, x11       // x10: current value of n
    movz x0, 1          // x0: accumulated product
    b    endinner
startinner:
    madd x0, x0, x10, xzr
    sub  x10, x10, 1
endinner:
    cbnz x10, startinner

    bl   intwrite
    bl   newlinewrite
endouter:
    add  x11, x11, 1
    cmp  x11, n
    b.ls outer

quit:
    movz x0, 0          // return value
    movz x8, 0x5d     // sys_exit service
    svc  0
//----------------------------------------------------------------
