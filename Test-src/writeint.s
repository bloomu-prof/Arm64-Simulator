// procedure calls, number conversion
// 2021-03-03
    .set n, 0xfe34

    .global _start
    .extern strwrite, intwrite
    .text
_start:
    ldr  x0, =msg1
    bl   strwrite

    movz x0, n
    bl   intwrite

    ldr  x0, =msg2
    bl   strwrite

    nop

    ldr  x0, =msg1
    bl   strwrite

    movz x0, n
    bl   hexwrite

    ldr  x0, =msg3
    bl   strwrite
    nop

    movz x0, 0          // return value
quit: movz x8, 0x5d     // sys_exit service
    nop
    svc  0
    .byte 0xee,0xee,0xee,0xee
the_end:
    .data
msg1:   .asciz "Number: "
msg2:   .asciz " (base 10)\n"
msg3:   .asciz " (hexadecimal)\n"
