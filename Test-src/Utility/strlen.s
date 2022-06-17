// assembly strlen()
// 2021-01-18
    .global strlen
    .text
// expects -
//   x0: pointer to string
// returns -
//   x0: string length, in bytes
strlen:
    mov  x1, x0         // copy of string pointer
strlen_each:
    ldrb w2, [x1], 1    // post-increment
    cbnz w2, strlen_each
    sub  x1, x1, 1      // undo last increment
    sub  x0, x1, x0     // string length = (end-start)
    ret
