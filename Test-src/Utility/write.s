// 2021-02-15
    .text
    .global write
// write() - emit a string to a filehandle
// expects -
//   x0: string buffer
//   x1: string's length
//   x2: file handle (sysout == 1, etc.)
write:
    stp  fp, lr, [sp, -0x10] !
    mov  fp, sp

    mov  x3, x0     // hafta reposition x0,x1,x2
    mov  w0, w2         // file_handle to x0
    mov  x2, x1         // string's length
    mov  x1, x3         // string
    movz x8, 0x40       // sys_write
    svc  0

    ldp  fp, lr, [sp], 0x10
    ret
//----------------------------------------------------------------

    .global syswrite
// syswrite() - emit a string to a filehandle
// expects -
//   x0: file handle (sysout == 1, etc.)
//   x1: string buffer
//   x2: string's length
syswrite:
    movz x8, 0x40       // sys_write
    svc  0
    ret
//----------------------------------------------------------------
