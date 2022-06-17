// 2021-03-08
// write-constant functions
    .global blankwrite, newlinewrite, syswrite

    .text
blankwrite:
    ldr  x1, =blank
    b    syswrite

newlinewrite:
    ldr  x1, =newline
    b    syswrite
//----------------------------------------------------------------

// syswrite() - emit a string to a filehandle
// expects -
//   x1: 1-char string buffer
syswrite:
    mov  x0, 1          // sysout
    movz x2, 1          // 1-char string
    movz x8, 0x40       // sys_write
    svc  0
    ret
//----------------------------------------------------------------

blank:      .ascii " "
newline:    .ascii "\n"
            .balign 4
//----------------------------------------------------------------
