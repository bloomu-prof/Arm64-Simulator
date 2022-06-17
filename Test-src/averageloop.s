// Calculate average of an array of integers
// 2020-12-06
    .global _start
    .extern int2str, write, blankwrite, newlinewrite, intwrite, strwrite
    .text

_start:
    ldr  x7, =array
    ldr  x1, =arrayend          // 1st address after array
    sub  x1, x1, x7             // minus start-address of array
    lsr  w1, w1, 2              // # of elements in array

    mov  w2, 0                  // initialize sum (dividend)
    mov  x4, 0                  // initialize loop counter
    b    looptest
loop:
    ldr  w5, [x7, x4, lsl 2]    // load next array element
    add  w2, w2, w5             // sum += array element
    add  w4, w4, 1              // increment loop counter
looptest:
    //subs wzr, w4, w1
    cmp  w4, w1                 // alias for "subs wzr, w4, w1"
    b.lo loop
loopdone:
    sdiv w3, w2, w1             // w3: quotient
    mul  w4, w3, w1
    sub  w4, w2, w4             // w4: remainder
outputs:
    mov  w21, w1        // n
    mov  w22, w2        // sum
    mov  w23, w3        // quotient
    mov  w24, w4        // remainder

    mov  x0, x21        // n
    bl   intwrite
    ldr  x0, =fmtn
    bl   strwrite

    mov  x0, x22        // sum
    bl   intwrite
    ldr  x0, =fmts
    bl   strwrite

    mov  w0, w23        // quotient
    bl   intwrite
    ldr  x0, =fmtq
    bl   strwrite

    mov  w0, w24        // remainder
    bl   intwrite
    ldr  x0, =fmtr
    bl   strwrite

quit:
    movz x0, 0          // return value
    movz x8, 0x5d     // sys_exit service
    svc  0

array:  .word 19
        .word -22
        .word 0
        .word 13
        .word -9
        .word 46
        .word -32
        .word 7
arrayend:
        .lcomm buffer, 65       // string buffer for conversions
fmtn:   .asciz " values:  "
fmts:   .asciz " sum,  "
fmtq:   .asciz " quotient,  "
fmtr:   .asciz " remainder\n"
