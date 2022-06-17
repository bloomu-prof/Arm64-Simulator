// Simple string printer
// 2021-01-18
    .global _start
    .extern strlen write
    .text
msg:    .asciz  "This...\n...is...\n...simplestring!\n" // message...
    .balign     4
_start:
    ldr  x0, =msg       // load the relocated "msg" address
    bl   strlen         // returns string's length

    orr  w1, w0, wzr    // copy the length into w1
    ldr  x0, =msg       // relocate the "msg" address
    movz x2, 1          // file handle
    bl   write

    movz x0, 0          // return value
quit: movz x8, 0x5d     // sys_exit service
    nop
    svc  0
//----------------------------------------------------------------
