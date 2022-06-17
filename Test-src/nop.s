    .text
    .global _start
commence:
    nop
    nop
    nop
_start:
    nop
    movz  w0, 0
    movz  x8, 0x5d
    svc   0
    nop
quit:
    nop
    nop
    nop
the_end:
