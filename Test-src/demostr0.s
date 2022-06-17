// simple LEGv8 program w/ C call
// 2020-12-06
    .global _start
    .extern strlen
    .text
    .set shft, 3
    .byte 0xee,0xee,0xee,0xee
filler: .asciz  "whoop-whoop"
msg:    .asciz  "test message-string\n" // message...
    .set  msglen, (. - msg)
    .balign     4
_start:

    movz x2, msglen
    ldr  x1, =msg       // relocate the "msg" address
    movz x0, 0x01       // stdout
    movz x8, 0x40       // sys_print service
prt: svc  0

    cmp  x0, xzr
    b.ne fooA
    b    fooB
fooA:
    ldr  x4, =msg
    mov  x3, 0
    ldr  x0, [x4, x3]
    ldr  x0, [x4, x3, lsl 3]    // pointless instructions
    ldr  x0, [x4, x3, lsl shft]
    ldr  w0, [x4, x3, lsl 2]
fooB:
    movz x0, 0          // return value
quit: movz x8, 0x5d     // sys_exit service
    nop
    svc  0
    .byte 0xee,0xee,0xee,0xee
the_end:
