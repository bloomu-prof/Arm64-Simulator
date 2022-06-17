// Recursive Fibonacci, ARMv8 assembly only
// 2021-01-18
    .text
    .set value, 12

    .global _start
    .extern intwrite, blankwrite, newlinewrite
_start:
    stp  x29, x30, [sp, -0x20] !
    mov  x29, sp

    mov  x0,xzr
    str  x0, [fp, 0x10] // save x0 as loop counter "n"
each:
    bl   intwrite
    bl   blankwrite
    bl   blankwrite

    ldr  x0, [fp, 0x10] // save "n" for later
    bl   fibo           // calculate fibo("n")
    bl   intwrite       // display this fibo("n")
    bl   newlinewrite

    ldr  x0, [fp, 0x10]
    add  x0, x0, 1
    str  x0, [fp, 0x10] // next loop-counter value
    cmp  x0, value
    b.ls each

quit:
    movz x0, 0          // return value
    movz x8, 0x5d     // sys_exit service
    svc  0
//----------------------------------------------------------------

// Recursive fibonacci function.
// fibo(n) = n, for n < 2.
// fibo(n) = fibo(n-1) + fibo(n-2) for n >= 2.
// expects -
//   x0: "n" - the nth Fibonacci number.
fibo:
    stp  x29, x30, [sp, -0x20] !
    mov  x29, sp

    str  x0, [x29, 0x10]    // save "n"
    str  x0, [x29, 0x18]    // "n" is default return value

    subs xzr, x0, 0x01
    b.ls basecase

    sub  x0, x0, 1
    bl   fibo               // fibo(n-1)
    str  x0, [x29, 0x18]    // save fibo(n-1)
    ldr  x0, [x29, 0x10]
    sub  x0, x0, 2
    bl   fibo               // fibo(n-2)
    ldr  x1, [x29, 0x18]    // retrieve fibo(n-1)
    add  x0, x1, x0
basecase:
    ldp  x29, x30, [sp], 0x20
    ret
//----------------------------------------------------------------
