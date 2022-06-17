// int2str
//      2021-03-08  "int2hex" procedure
    .text
    .global int2hex

// Convert an integer to a Hex ASCIZ string
// using a simple algorithm.
//
// expects:
//   x0 - target string-buffer's address
//   x1 - integer to be converted
// returns:
//   x0 - target string-buffer's address
int2hex:
    ldr  x2, =digits
    add  x3, x0, 16
    strb wzr, [x3]      // terminating NUL character at end
each_digit:
    and  x4, x1, #0x0f
    ldrb w5, [x2, x4]
    strb w5, [x3, -1] !
    lsr  x1, x1, 4
    cmp  x3, x0
    b.hs  each_digit
    ret
digits: .ascii "0123456789abcdefghijklmnopqrstuvwxyz"
//--------

