//   writeint() - convert integer to string, & display
// 2021-03-09
// 2021-02-19
// 2021-01-18
    .set  FrameSize, 0x10
    .global intwrite
    .extern int2str
    .text
// expects -
//   x0: value to write
    .lcomm cvtbuffer, 68
    .balign 4
intwrite:
    stp  fp, lr, [sp, -FrameSize] !
    mov  fp, sp

    mov  x1, x0     // value to x1
    movz x2, 10     // base-10 by default
    ldr  x0, =cvtbuffer
    bl   int2str
    mov  x2, x0         // string length
    ldr  x1, =cvtbuffer
    movz x0, 1          // sysout
    movz x8, 0x40       // sys_write
    svc  0
    ldp  fp, lr, [sp], FrameSize
    ret
//----------------------------------------------------------------
