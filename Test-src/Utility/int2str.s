// int2str
    .text
    .global int2str, reverse

digits: .ascii "0123456789abcdefghijklmnopqrstuvwxyz"
    .balign 4

// Convert an integer to an ASCIZ string
// expects:
//   x0 - target string-buffer's address
//   x1 - integer to be converted
//   x2 - desired base
// returns:
//   x0 - length of string
    .set FrameSize, 0x20
int2str:
    stp  fp, lr, [sp, -FrameSize] !
    mov  fp, sp
    str  x0, [fp, 0x10]     // preserve buffer addr, src integer...
    //str  x2, [fp, 0x20]         // ...and desired base

    ldr  x9, =digits
    mov  x5, x0         // copy string-pointer for iterating
do_division:
    //ldr  x10, [fp, 0x20]
    udiv x10, x1, x2    // new quotient
    mul  x3, x10, x2    // calculate...
    sub  x3, x1, x3     // ...the remainder
    ldrb w4, [x9, x3]   // get ascii digit
    strb w4, [x5], 1    // post-increment to next empty pos'n

    mov  x1, x10        // update dividend
    cbnz x1, do_division
    strb wzr, [x5], -1  // NUL-terminate, & back to last digit

    mov  x1, x5         // end-pointer into x1 (start-pointer in x0)
    bl   reverse

    ldr  x0, [fp, 0x10] // restore start-pointer
    sub  x0, x5, x0     // calculate string length...
    add  x0, x0, 1      // ...as (end - start + 1)

    ldp  fp, lr, [sp], FrameSize
    ret
//--------

// expects:
//   x0 - string start-pointer
//   x1 - string end-pointer
reverse:
    ldrb w6, [x1]       // last char
    ldrb w7, [x0]       // 1st char
    strb w6, [x0], 1    // store, advance to next start-char
    strb w7, [x1], -1   // store, backup to next end-char
    cmp  x0, x1
    b.lo reverse
    ret
//--------
