//------------------------------
// Display large values
// 2021-03-08
    .set value, 0xcafe

    .global _start
    .extern hexwrite, newlinewrite
    .text
_start:

    // output the desired value:

    movz x0, value
    bl   hexwrite
    bl   newlinewrite

    // output a message about expected output:

    movz x2, chkmsglen
    ldr  x1, =chkmsg    // relocate the "msg" address
    movz x0, 0x01       // stdout
    movz x8, 0x40       // sys_print service
prt: svc  0

quit:
    movz x0, 0          // return value
    movz x8, 0x5d     // sys_exit service
    nop
    svc  0
    .byte 0xee,0xee,0xee,0xee
the_end:
    .data
    .lcomm hexstring, 17
chkmsg:
    .asciz "    should be 0x000000000000cafe\n"
    .set  chkmsglen, (. - chkmsg)
//------------------------------
