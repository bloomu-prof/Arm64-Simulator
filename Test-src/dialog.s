// Write a prompt, read a response, write a final reply.
// Deal with nul-terminated ("ASCIZ") strings for outputs.
// 2022-05-29

.set SYSin,  0x00
.set SYSout, 0x01
.set SYSerr, 0x02

.set SYS_read,   0x3f
.set SYS_write,  0x40
.set SYS_open, 0x0400
.set SYS_close,    56
.set SYS_exit,   0x5d


    .global _start
    .extern strlen write

    .data
prompt:     .asciz  "Who are you? "
greeting:   .asciz  "What's up, "
grt2:       .asciz  "?\n"
.set grt2len, (. - grt2)

    .bss                // "Block Started by Symbol" - empty storage
    .balign 4           // align storage on 4-byte boundaries
    .lcomm inputlen, 8            // space for input's length
.set MAXSIZE, 256               // reasonable maximum input?
    .lcomm buffer, MAXSIZE      // space for the input

    .text
    .balign     4
_start:
// prompt for input:
    ldr  x0, =prompt    // load the relocated "prompt" address
    bl   strlen         // returns prompt's length

    orr  w1, w0, wzr    // copy the length into w1
    ldr  x0, =prompt    // relocate the "prompt" address (again)
    movz x2, SYSout     // file handle
    bl   write

// get input:
    mov x0, SYSin
    ldr x1, =buffer             // (assembler converts to .text-segment location)
    mov x2, MAXSIZE
    mov x8, SYS_read
    svc #0
    ldr x1, =inputlen           // (assembler converts to .text-segment location)
    str x0, [x1]                // save entered length

// write reply:
    ldr  x0, =greeting  // load the relocated "prompt" address
    bl   strlen         // returns prompt's length

    // Use a "helper" routine to do the output
    orr  w1, w0, wzr    // copy the length into w1
    ldr  x0, =greeting  // relocate the "prompt" address (again)
    movz x2, SYSout     // file handle
    bl   write

    // Echo back the input, directly
    mov  w0, SYSout     // file_handle to x0
    ldr  x2, =inputlen  // string's length
    ldr  x2, [x2]       // string's length
    sub  x2, x2, 1      // omit terminal newline
    ldr  x1, =buffer    // string
    movz x8, SYS_write
    svc  0

    movz w0, SYSout
    ldr  x1, =grt2      // relocate the "prompt" address (again)
    mov  w2, grt2len    // constant length
    movz x8, SYS_write
    svc  0

// finish
quit:
    movz x0, 0          // return value
    movz x8, SYS_exit
    svc  0
//----------------------------------------------------------------
