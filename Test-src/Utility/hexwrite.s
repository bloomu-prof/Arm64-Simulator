// 2021-03-08
    .set  FrameSize, 0x10

    .text
    .global hexwrite
// expects -
//   x0: value to write
hexwrite:
    stp  fp, lr, [sp, -FrameSize] !
    mov  fp, sp

    mov  x1, x0     // value to x1
    ldr  x0, =cvtbuffer
    bl   int2hex
    movz x2, 16         // string length
    ldr  x1, =cvtbuffer
    movz x0, 1          // sysout
    movz x8, 0x40       // sys_write
    svc  0

    ldp  fp, lr, [sp], FrameSize
    ret
    .lcomm cvtbuffer, 17
//----------------------------------------------------------------
