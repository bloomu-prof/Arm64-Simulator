//   writestr() - display NUL-terminated string
// 2021-03-09
// 2021-02-19
// 2021-01-18
    .set  FrameSize, 0x10
    .global strwrite
    .extern strlen
    .text
// expects -
//   x0: asciz string to write
strwrite:
    stp  fp, lr, [sp, -2*FrameSize] !
    mov  fp, sp
    str  x0, [fp, 0x10]
    bl   strlen
    mov  x2, x0     // length to x1
    ldr  x1, [fp, 0x10]
    movz x0, 1      // sysout
    movz x8, 0x40       // sys_write
    svc  0
    ldp  fp, lr, [sp], FrameSize
    ret
//----------------------------------------------------------------
