//------------------------------
// Display large values
// 2021-03-08
    .set lsbits, 0x10cb
    .set middleL, 0x5432
    .set middleM, 0x9876
    .set msbits, 0xfade

    .global _start
    .extern hexwrite, newlinewrite
    .text
_start:

    // output the desired value:

    movz x0, lsbits
    movk x0, middleL, lsl 16
    movk x0, middleM, lsl 32
    movk x0, msbits, lsl 48
    bl   hexwrite
    bl   newlinewrite

    // output a message about expected output:

    movz x2, chkmsglen
    ldr  x1, =chkmsg    // relocate the "msg" address
    movz x0, 0x01       // stdout
    movz x8, 0x40       // sys_print service
prt: svc  0

    movz x0, 0          // return value
quit: movz x8, 0x5d     // sys_exit service
    nop
    svc  0
    .byte 0xee,0xee,0xee,0xee
the_end:
    .data
    .lcomm hexstring, 17
chkmsg:
    .asciz "    should be 0xfade986543210cb\n"
    .set  chkmsglen, (. - chkmsg)
//    .set lsbits, 0x10cb
//    .set middleL, 0x5432
//    .set middleM, 0x9876
//    .set msbits, 0xfade
//------------------------------
