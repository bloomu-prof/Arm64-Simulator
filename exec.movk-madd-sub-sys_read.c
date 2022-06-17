/*
* execute.c - simulate execution of an instruction
* 2022-05-27 v3.0 Implement interactive/batch modes (no effect on this file).
* 2022-05-23 v2.0 Add missing instructions, tagged with "#ifdef ALL"
*   sub_sh, sub_32
*   madd, mul_64, mul_32
*   movk
* 2021-04-14 v1.1 Simplify the add_i implementations
* 2021-03-02 v1.0
*/
#include <stdio.h>
#include <unistd.h>     // write()
#include <string.h>     // strcmp()
#include "cpu.h"

#define ALL

/*
* Test registers, set the global APSR status register appropriately.
*   Primarily used by "subtract" instructions;
*   would also be used by "cmp" if that gets implemented.
*/
void set_apsr(long int ALUout, long int ALUinN, long int ALUinM)
{
    apsr.zero = (ALUout == 0);
    apsr.negative = (ALUout < 0);
    apsr.overflow = (
        (ALUinN < 0 && ALUinM > 0 && ALUout >= 0)
        || (ALUinN > 0 && ALUinM < 0 && ALUout < 0)
    );
    apsr.carry = !( ALUinN < ALUinM );
}

/*
* Compute the "roll-right" amount for a register's value.
*/
long unsigned roll_right(long unsigned reg, unsigned size, unsigned shift)
{
    if (shift == 0)
        return reg;
    else {
        return  ( (reg >> shift) | (reg << (size - shift)) );
    }
}

/*
* Copied almost verbatim from
*   https://developer.arm.com/documentation/ddi0596/2020-12/Shared-Pseudocode/AArch64-Instrs?lang=en#impl-aarch64.DecodeBitMasks.4
*/
long unsigned decode_bit_mask_w(
    short unsigned N, short unsigned imms, short unsigned immr,
    short unsigned is_immediate)
{
    unsigned not_imms = N << 6 | ~imms;
    short unsigned len;
    for (int i = 6; i >= 0; i--) {
        if (not_imms & 0x01 << i) {
            len = i;
            break;
        }
        len = -1;
    }
    if (debug) {
        fprintf(logout, "decode_bit_mask_w: not_imms %#x  len %#x\n", not_imms, len);
    }
    short unsigned levels = 0x03f & ((1<<len) - 1);    // zero-extend len to 6 bits
    unsigned S = imms & levels;
    unsigned R = immr & levels;
    //int diff = S - R;   // 6-bit subtract w/ borrow
    if (debug) {
        fprintf(logout, "decode_bit_mask_w: levels %#x  S %#x  R %#x\n",
            levels, S, R);
    }
    short unsigned esize = 1 << len;
    short unsigned welem = ~(0xffffffff << (S+1));    // ZeroExtend(Ones(S+1), esize)
    long unsigned wmask = roll_right(welem, esize, R);
    if (debug) {
        fprintf(logout, "decode_bit_mask_w: esize %#x  welem %#x  wmask %#lx\n",
            esize, welem, wmask);
    }
    return wmask;
}

/*
* Implement the Execute stage of the datapath.
*   For coding convenience and readability, choose an action based
*   on the decoded mnemonic rather than the binary opcode itself.
*   Some of intermediate values calculated here might more reasonably
*   be calculated in the Decode stage, but it's simpler to calculate
*   them just before they're actually used...
*/
void execute(Instruction *ir, Memory *program)
{
    unsigned instr = ir->instruction.value;
    long int address;
    long unsigned size_mask;
    long int ALUout, ALUinN, ALUinM;
    if (ir->regsize == 32) {
        size_mask = 0x00000000ffffffff;
    } else if (ir->regsize == 64) {
        size_mask = 0xffffffffffffffff;
    } else {
        fprintf(logout, "BAD REGSIZE %#x\n", ir->regsize);
        fflush(NULL);
        size_mask = 0x0;
    }
    ALUout = ir->regsize_mask & registers[ir->rd].dword;
    ALUinN = ir->regsize_mask & registers[ir->rn].dword;
    ALUinM = ir->regsize_mask & registers[ir->rm].dword;
    if (debug) {
        fprintf(logout, "  ALUout=%#08lx  ALUinN=%#08lx  ALUinM=%#08lx\n",
            ALUout, ALUinN, ALUinM);
        fflush(NULL);
    }

    if (!strcmp(ir->mnemonic, "nop")) {
        fprintf(logout, "\n%s (DO NOTHING)\n", ir->mnemonic);
        if (print)
            display_memory(program);

    //---- ALU operations:

    } else if (!strcmp(ir->mnemonic, "add_32") || !strcmp(ir->mnemonic, "add_64")) {
        registers[ir->rd].dword = size_mask & (ALUinN + ALUinM);

    } else if (!strcmp(ir->mnemonic, "add_i")) {
        long unsigned result;
        ALUinN = (ir->rn == 31  ?  stack_pointer  :  registers[ir->rn].dword);
        ALUinM = (ir->lshift  ?  (ir->uimm12) << 12  :  ir->uimm12);

        result = (ir->regsize_mask & ALUinN) + (ir->regsize_mask & ALUinM);

        if (debug) {
            fprintf(logout, "  ALUinN %#lx  ALUinM %#lx  result %#lx\n",
                ALUinN, ALUinM, result);
        }
        if (ir->rd == 31)
            stack_pointer = result;
        else
            registers[ir->rd].dword = result;

    } else if (!strcmp(ir->mnemonic, "orr_i")) {
        if (debug) {
            fprintf(logout, "  %s  immr %#010x  imms %#010x\n",
                ir->mnemonic, ir->immr, ir->imms);
            fprintf(logout, "  %s  regsize_mask %#010lx   Rn %#x  reg[Rn] %#lx\n",
                ir->mnemonic, ir->regsize_mask, ir->rn, registers[ir->rn].dword);
        }
        long unsigned result;
        unsigned N = (instr & (1 << 22));
        ALUinN = registers[ir->rn].dword;
        ALUinM = (ir->imms << 6) | ir->immr;
        ALUinM |= N << 12;
        ALUinM = (ir->lshift  ?  (ir->uimm12) << 12  :  ir->uimm12);
        result = (ir->regsize_mask & ALUinN) | (ir->regsize_mask & ALUinM);
        if (ir->rd == 31)
            stack_pointer = result;
        else
            registers[ir->rd].dword = result;


    } else if (!strcmp(ir->mnemonic, "and_i")) {
        long unsigned result;
        unsigned N = (instr & (1 << 22));
        ALUinN = registers[ir->rn].dword;

        long unsigned imm = decode_bit_mask_w(N, ir->imms, ir->immr, 1);
        result = (ir->regsize_mask & ALUinN) & imm;
        if (debug) {
            fprintf(logout, "  immr %#x  imms %#x  imm %#lx\n",
                ir->immr, ir->imms, imm);
            fprintf(logout, "  result %#lx  ALUinN %#lx\n",
                result, (ir->regsize_mask & ALUinN));
        }
        if (ir->rd == 31)
            stack_pointer = result;
        else
            registers[ir->rd].dword = result;

    } else if (!strcmp(ir->mnemonic, "orr")) {
        long unsigned result;
        //unsigned N = (instr & (1 << 21));
        ALUinN = registers[ir->rn].dword;
        ALUinM = registers[ir->rm].dword;
        switch (ir->shift) {
          case 0:
            ALUinM <<= ir->shamt;
            break;
          case 1:
            ALUinM >>= ir->shamt;
            break;
          case 2:
            fprintf(logout, "ASR shift_type %#x not implemented\n", ir->shift);
            break;
          case 3:
            fprintf(logout, "ROR shift_type %#x not implemented\n", ir->shift);
            break;
          default:
            fprintf(logout, "bad shift_type %#x !\n", ir->shift);
        }
        result = (ir->regsize_mask & ALUinN) | (ir->regsize_mask & ALUinM);
        if (ir->rd == 31)
            stack_pointer = result;
        else
            registers[ir->rd].dword = result;

    /*
    *-------- variations on "subtract" --------
    */

    } else if (!strcmp(ir->mnemonic, "subs_sh")) {
        unsigned shift_amount = ir->uimm6;  // do in decode?
        ALUinN &= ir->regsize_mask;
        switch (extract_middle(23, 22, instr)) {
          case 0:   // LSL
            ALUinM = (ir->regsize_mask & ALUinM) << shift_amount;
            break;
          case 1:   // LSR
            ALUinM = (ir->regsize_mask & ALUinM) >> shift_amount;
            break;
          case 2:   // ASR
            ALUinM = (ir->regsize_mask & ALUinM) >> shift_amount;
            break;
          default:
            fprintf(logout, "\nsubs_sh: bad shift choice %#x\n",
                extract_middle(23, 22, instr));
        }
        ALUout = ALUinN - ALUinM;
        if (debug) {
            fprintf(logout, "subs_sh:  ALUout %#lx\n", ALUout);
        }
        set_apsr(ALUout, ALUinN, ALUinM);
        registers[ir->rd].dword = ALUout;

    } else if (!strcmp(ir->mnemonic, "subs_i")) {
        ALUinN = (ir->regsize_mask & ALUinN);
        ALUinM = (ir->lshift  ?  (ir->uimm12) << 12  :  ir->uimm12);
        ALUout = ALUinN + (-ALUinM);
        if (debug) {
            fprintf(logout, "  subs_i: ALUout %#lx\n", ALUout);
        }
        set_apsr(ALUout, ALUinN, ALUinM);
        registers[ir->rd].dword = ALUout;

    } else if (!strcmp(ir->mnemonic, "sub_i")) {
        ALUinN = (ir->regsize_mask & ALUinN);
        ALUinM = (ir->lshift  ?  (ir->uimm12) << 12  :  ir->uimm12);
        ALUout = ALUinN + (-ALUinM);
        if (debug) {
            fprintf(logout, "  sub_i: ALUout %#lx\n", ALUout);
        }
        set_apsr(ALUout, ALUinN, ALUinM);
        registers[ir->rd].dword = ALUout;

#ifdef ALL
    } else if (!strcmp(ir->mnemonic, "sub_sh")) {
        unsigned shift_amount = ir->uimm6;  // do in decode?
        ALUinN &= ir->regsize_mask;
        switch (extract_middle(23, 22, instr)) {
          case 0:   // LSL
            ALUinM = (ir->regsize_mask & ALUinM) << shift_amount;
            break;
          case 1:   // LSR
            ALUinM = (ir->regsize_mask & ALUinM) >> shift_amount;
            break;
          case 2:   // ASR
            ALUinM = (ir->regsize_mask & ALUinM) >> shift_amount;
            break;
          default:
            fprintf(logout, "\nsub_sh: bad shift choice %#x\n",
                extract_middle(23, 22, instr));
        }
        ALUout = ALUinN - ALUinM;
        if (debug) {
            fprintf(logout, "sub_sh:  ALUout %#lx\n", ALUout);
        }
        registers[ir->rd].dword = ALUout;

    } else if (!strcmp(ir->mnemonic, "sub_32") || !strcmp(ir->mnemonic, "sub_64")) {
        ALUinN = (ir->regsize_mask & ALUinN);
        ALUout = ALUinN - ALUinM;
        if (debug)
            fprintf(logout, "  %s:  ALUout %#lx\n", ir->mnemonic, ALUout);
        fflush(NULL);
        registers[ir->rd].dword &= (~ir->regsize_mask);
        registers[ir->rd].dword |= (ir->regsize_mask & ALUout);
        if (debug)
            fprintf(logout, "  stored: %#lx\n", registers[ir->rd].dword);
        fflush(NULL);
    /*
    *------------------------------------------------
    */
#endif

    //----  ubfm / lsl / lsr  ----

    } else if ( !strcmp(ir->mnemonic, "ubfm")
                || !strcmp(ir->mnemonic, "lsl")
                || !strcmp(ir->mnemonic, "lsr")
    ) {
        long unsigned src = ir->regsize_mask & registers[ir->rn].dword;
        //long unsigned wmask, tmask; // pseudocode for these is WEIRD.
        //ir->rd = (roll_right(src, ir->regsize, ir->immr) & wmask) & tmask;
        registers[ir->rd].dword = roll_right(src, ir->regsize, ir->immr);
        if (debug) {
            //fprintf(logout, "  %s  immr %#010x  imms %#010x  wmask %#x  tmask %#x\n",
            //    ir->mnemonic, ir->immr, ir->imms, wmask, tmask);
            fprintf(logout, "  %s  src %#010lx  regsize_mask %#010lx   Rn %#x\n",
                ir->mnemonic, src, ir->regsize_mask, ir->rn);
            fprintf(logout, "    immr %#010x  imms %#010x   Rd %#018x\n",
                ir->immr, ir->imms, ir->rd);
        }


    //---- Divides ----

    } else if (!strcmp(ir->mnemonic, "udiv_64")) {
        if (debug)
            fprintf(logout, "  udiv: ALUinN=%#lx  ALUinM=%#lx\n", ALUinN, ALUinM);
        registers[ir->rd].dword = (long unsigned)ALUinN / (long unsigned)ALUinM;

    } else if (!strcmp(ir->mnemonic, "sdiv_64")) {
        if (debug)
            fprintf(logout, "  sdiv: ALUinN=%#lx  ALUinM=%#lx\n", ALUinN, ALUinM);
        registers[ir->rd].dword = (long int)ALUinN / (long int)ALUinM;

    } else if (!strcmp(ir->mnemonic, "udiv_32")) {
        if (debug)
            fprintf(logout, "  udiv: ALUinN=%#lx  ALUinM=%#lx\n", ALUinN, ALUinM);
        registers[ir->rd].dword = (unsigned)ALUinN / (unsigned)ALUinM;

    } else if (!strcmp(ir->mnemonic, "sdiv_32")) {
        if (debug)
            fprintf(logout, "  sdiv: ALUinN=%#lx  ALUinM=%#lx\n", ALUinN, ALUinM);
        registers[ir->rd].dword = (int)ALUinN / (int)ALUinM;

    /*
    *-------- Multiplys --------
    */
#ifdef ALL
    } else if (!strcmp(ir->mnemonic, "madd")) {
        long unsigned operand1, operand2, operand3;

        operand1 = ir->regsize_mask & registers[ir->rn].dword;
        operand2 = ir->regsize_mask & registers[ir->rm].dword;
        operand3 = ir->regsize_mask & registers[ir->rt2].dword;
        registers[ir->rd].dword = operand3 + operand1 * operand2;
        if (debug)
            fprintf(logout, "madd: r[%#x] / %#lx = %#lx + %#lx * %#lx\n",
                ir->rd, registers[ir->rd].dword, operand3, operand1, operand2);

    } else if (!strcmp(ir->mnemonic, "mul_64")) {
        registers[ir->rd].dword = (long unsigned)ALUinN * (long unsigned)ALUinM;

    } else if (!strcmp(ir->mnemonic, "mul_32")) {
        registers[ir->rd].dword = (unsigned)ALUinN * (unsigned)ALUinM;
#endif

    //---- MOV operations ----

    } else if (!strcmp(ir->mnemonic, "movz")) {
        if (debug)
            fprintf(logout, "  %s - w%#x <- %#lx\n",
                ir->mnemonic, ir->rd, ir->imm16);
        unsigned hword_count = 0;
        if (ir->regsize == 32)
            hword_count = 2;
        else if (ir->regsize == 64)
            hword_count = 4;
        else
            fprintf(logout, "movz: broken ir->regsize %#x\n", ir->regsize);

        // Zero out the unused portion of the register
        for (unsigned i = 0; i < hword_count; i++)
            registers[ir->rd].hword[i] = 0x0;
        // Extract the target byte(s) position from the instruction bits:
        unsigned const_posn = extract_middle(22, 21, instr);
        // Place the constant value from the instruction into the proper part
        //  of the register.
        registers[ir->rd].hword[ const_posn ] = ir->imm16;

#ifdef ALL
    /*
    * Add "movk", needed by one of the conversion utilities:
    */
    } else if (!strcmp(ir->mnemonic, "movk")) {
        if (debug)
            fprintf(logout, "  %s - w%d <- %#lx\n",
                ir->mnemonic, ir->rd, ir->imm16);

        // Extract the target byte(s) position from the instruction bits:
        unsigned const_posn = extract_middle(22, 21, instr);
        registers[ir->rd].hword[ const_posn ] = ir->imm16;

        if (debug) {
            fprintf(logout, "  const_posn %#x  reg[rd].word[const_posn] %#x\n",
                const_posn, registers[ir->rd].word[ const_posn ]);
        }
#endif

    //---- Memory loads ----

    // 3 forms of ldrb_i: post-increment, pre-increment, unsigned-offset
    } else if (!strcmp(ir->mnemonic, "ldrb_i")) {
        int writeback = ! extract_middle(24, 24, instr);
        int postindex = ! extract_middle(11, 11, instr);
        long int offset = (writeback) ? ir->simm9 : ir->uimm12;
        if (debug)
            fprintf(logout,
                "  %s - Rn %#x,  Rt %#x, simm9 %#lx, uimm12 %#lx, offset %#lx\n",
                ir->mnemonic, ir->rn, ir->rt, ir->simm9, ir->uimm12, offset);

        address = (ir->rn == 31) ? stack_pointer : registers[ir->rn].dword;
        if (!postindex)
            address += offset;
        registers[ir->rt].dword = 0x00;
        accessMem(program, registers[ir->rt].bytes, 'r', address, 1);

        if (writeback) {
            if (ir->rn == 31)
                stack_pointer += offset;
            else
                registers[ir->rn].dword += offset;
        }

    } else if (!strcmp(ir->mnemonic, "ldrb_reg")) {    // offset the register
        int offset = registers[ir->rm].dword;
        address = (ir->rn == 31) ? stack_pointer : registers[ir->rn].dword;

        registers[ir->rt].dword = 0x00; // zero the whole register first
        // load a byte into the desired part of the "rt" register:
        accessMem(program, registers[ir->rt].bytes, 'r', (address + offset), 1);

    } else if (!strcmp(ir->mnemonic, "ldr_i")) {
        if (debug) {
            fprintf(logout, "  %s - Rn %#x, Rt %#x\n",
                ir->mnemonic, ir->rn, ir->rt);
            fprintf(logout, "  %s - simm9 %#lx, uimm12 %#lx  v[21:10] %#x\n",
                ir->mnemonic, ir->simm9, ir->uimm12,
                extract_middle(21, 10, instr));
        }
        int scale = extract_n_upper(2, instr);
        int datasize = 0x8 << scale;
        int regsize = (0x3 == scale  ?  64  :  32);
        int post = (0x1 == extract_middle(11, 10, instr));
        int pre = (0x3 == extract_middle(11, 10, instr));
        int prepost = (0x0 == extract_middle(24, 24, instr));
        int offset = (prepost  ?  ir->simm9  :  (ir->uimm12 << scale));
        if (debug) {
            fprintf(logout, "  %s - scale %#x  datasize %#x  regsize %#x\n",
                ir->mnemonic, scale, datasize, regsize);
            fprintf(logout, "  %s - pre %#x  post %#x  prepost %#x  offset %#x\n",
                ir->mnemonic, pre, post, prepost, offset);
        }
        if (ir->rn == 31) {
            address = stack_pointer;
            if (prepost)
                stack_pointer += offset;
        } else {
            address = registers[ir->rn].dword;
            if (prepost)
                registers[ir->rn].dword += offset;
        }
        if (!post)
            address += offset;
        accessMem(program, registers[ir->rt].bytes, 'r', address, datasize>>3);

    } else if (!strcmp(ir->mnemonic, "ldr_64n")) {        // register-iNdirect
        address = program_counter + ir->rm;
        accessMem(program, registers[ir->rt].bytes, 'r', address, 8);

    } else if (!strcmp(ir->mnemonic, "ldr_reg")) {        // register
        //  1?111000011.....???S10..........
        unsigned short scale = ir->sizebits;
        unsigned datasize = 0x8 << scale;
        // Determine "extend" option:
        //short unsigned option = extract_middle(15, 13, instr);
        short unsigned shift =
            (extract_middle(12, 12, instr) == 1)  ?  scale  :  0;
        long unsigned offset = registers[ir->rm].dword << shift;
        address = (ir->rn == 31)  ?  stack_pointer  :  registers[ir->rn].dword;
        if (debug) {
            fprintf(logout, "  %s  address %#lx  offset %#lx\n",
                ir->mnemonic, address, offset);
        }
        accessMem(program, registers[ir->rt].bytes, 'r', (address + offset), datasize>>3);

    } else if (!strcmp(ir->mnemonic, "ldr_pc64")) {       // pc-relative
        unsigned offset = (ir->imm19)<<2;
        address = program_counter+ offset;
        accessMem(program, registers[ir->rt].bytes, 'r', address, 8);
        if (debug)
            fprintf(logout, "  execute \"%s\" x%d <- memory\n", ir->mnemonic, ir->rt);

    } else if (!strcmp(ir->mnemonic, "ldr_pc32")) {       // pc-relative
        unsigned offset = (ir->imm19)<<2;
        address = program_counter + offset;
        accessMem(program, registers[ir->rt].bytes, 'r', address, 4);
        for (int i = 4; i < 8; i++)
            registers[ir->rt].bytes[i] = 0;
        if (debug)
            fprintf(logout, "  execute \"%s\" x%d <- memory\n", ir->mnemonic, ir->rt);

    } else if (!strcmp(ir->mnemonic, "ldr_pc32s")) {      // pc-relative, sign-extension
        address = program_counter + ir->imm19;
        accessMem(program, registers[ir->rt].bytes, 'r', address, 4);
        // USE HIGHEST-ORDER SIGN BIT !!!
        signed char signbits = ((registers[ir->rt].bytes[3] & 0x80) ? 0xff : 0);
        for (int i = 4; i < 8; i++) {
            registers[ir->rt].bytes[i] = signbits;
        }

    } else if (!strcmp(ir->mnemonic, "ldp")) {  // also handles "ldnp"
        if (debug) {
            fprintf(logout, "  %s - Rn %#x  Rt %#x  Rt2 %#x, simm7 %#lx\n",
                ir->mnemonic, ir->rn, ir->rt, ir->rt2, ir->simm7);
        }
        unsigned is_signed = extract_middle(30, 30, (unsigned)(instr));
        unsigned scale = 2 + extract_n_upper(1, instr);
        unsigned datasize = 0x8 << scale;
        unsigned databytes = datasize >> 3;
        long int offset = (ir->simm7 << scale);
        unsigned prepost = extract_middle(24, 23, instr);
        if (debug) {
            fprintf(logout,
                "  %s - scale %#x  datasize %#x   prepost %#x  is_signed %#x\n",
                ir->mnemonic, scale, datasize, prepost, is_signed);
        }
        if (ir->rn == 31) {
            address = stack_pointer;
            if (prepost & 0x1)
                stack_pointer += offset;
        } else {
            address = registers[ir->rn].dword;
            if (prepost & 0x1)
                registers[ir->rn].dword += offset;
        }
        if (prepost & 0x2)
            address += offset;
        if (debug)
            fprintf(logout, "  %s - offset %#lx  address %#lx\n",
                ir->mnemonic, offset, address);

        if (!is_signed) {
            accessMem(program, registers[ir->rt].bytes, 'r', address, datasize>>3);
            accessMem(program, registers[ir->rt2].bytes, 'r', address + databytes, datasize>>3);
        } else {
            // not correct - but is it moot?
            accessMem(program, registers[ir->rt].bytes, 'r', address, datasize>>3);
            accessMem(program, registers[ir->rt2].bytes, 'r', address + databytes, datasize>>3);
        }


    //---- Memory stores ----

    // 3 forms of strb_i: post-increment, pre-increment, unsigned-offset
    } else if (!strcmp(ir->mnemonic, "strb_i")) {     // pre-increment the register
        int writeback = ! extract_middle(24, 24, instr);
        int postindex = ! extract_middle(11, 11, instr);
        long int offset = (writeback) ? ir->simm9 : ir->uimm12;
        if (debug)
            fprintf(logout,
                "  %s - Rn %#x,  Rt %#x, simm9 %#lx, uimm12 %#lx, offset %#lx\n",
                ir->mnemonic, ir->rn, ir->rt, ir->simm9, ir->uimm12, offset);

        address = (ir->rn == 31) ? stack_pointer : registers[ir->rn].dword;
        if (!postindex)
            address += offset;
        accessMem(program, registers[ir->rt].bytes, 'w', address, 1);

        if (writeback) {
            if (ir->rn == 31)
                stack_pointer += offset;
            else
                registers[ir->rn].dword += offset;
        }

    } else if (!strcmp(ir->mnemonic, "strb_reg")) {     // register-offset
        //  00111000001.....oooS10..........
        // Determine "extend" option:
        //short unsigned option = extract_middle(15, 13, instr);
        long unsigned offset = registers[ir->rm].dword;
        address = (ir->rn == 31) ? stack_pointer : registers[ir->rn].dword;
        accessMem(program, registers[ir->rt].bytes, 'w', (address + offset), 1);

    } else if (!strcmp(ir->mnemonic, "str_reg")) {     // register-offset
        //  1.111000001.....oooS10..........
        unsigned short scale = ir->sizebits;
        unsigned datasize = 0x8 << scale;
        // Determine "extend" option:
        //short unsigned option = extract_middle(15, 13, instr);
        short unsigned shift =
            (extract_middle(12, 12, instr) == 1)  ?  scale  :  0;
        long unsigned offset = registers[ir->rm].dword << shift;
        address = (ir->rn == 31) ? stack_pointer : registers[ir->rn].dword;
        accessMem(program, registers[ir->rt].bytes, 'w', (address + offset), datasize>>3);

    } else if (!strcmp(ir->mnemonic, "str_i")) {    // base register + offset
        long unsigned scale = extract_n_upper(2, instr);
        if (ir->rn == 31) {
            address = stack_pointer;
        } else {
            address = registers[ir->rn].dword;
        }
        address += ir->uimm12 << scale;
        if (debug) {
            fprintf(logout, "  %s - Rn %#x,  Rt %#x, uimm12 %#lx,  address %#lx\n",
                ir->mnemonic, ir->rn, ir->rt, ir->uimm12, address);
            fflush(NULL);
        }
        accessMem(program, registers[ir->rt].bytes, 'w', address, 8);


    } else if (!strcmp(ir->mnemonic, "str_64pre")) {      // pre-increment the register
        if (debug)
            fprintf(logout, "  %s - Rn %#x, simm9 %#lx\n",
                ir->mnemonic, ir->rn, ir->simm9);
        if (ir->rn == 31) {
            stack_pointer += (ir->simm9 << 3);
            address = stack_pointer;
        } else {
            registers[ir->rn].dword += (ir->simm9 << 3);
            address = registers[ir->rn].dword;
        }
        accessMem(program, registers[ir->rt].bytes, 'w', address, 8);

    } else if (!strcmp(ir->mnemonic, "str_64post")) {     // post-increment the register
        if (ir->rn == 31) {
            address = stack_pointer;
        } else {
            address = registers[ir->rn].dword;
        }
        accessMem(program, registers[ir->rt].bytes, 'w', address, 8);
        if (ir->rn == 31) {
            stack_pointer += (ir->simm9 << 3);
        } else {
            registers[ir->rn].dword += (ir->simm9 << 3);
        }

    } else if (!strcmp(ir->mnemonic, "str_32pre")) {    // pre-increment the register
        if (debug)
            fprintf(logout, "  %s - Rn %#x, simm9 %#lx\n",
                ir->mnemonic, ir->rn, ir->simm9);
        if (ir->rn == 31) {
            stack_pointer += (ir->simm9 << 2);
            address = stack_pointer;
        } else {
            registers[ir->rn].dword += (ir->simm9 << 2);
            address = registers[ir->rn].dword;
        }
        accessMem(program, registers[ir->rt].bytes, 'w', address, 8);

    } else if (!strcmp(ir->mnemonic, "str_32post")) {     // pre-increment the register
        if (ir->rn == 31) {
            address = stack_pointer;
        } else {
            address = registers[ir->rn].dword;
        }
        accessMem(program, registers[ir->rt].bytes, 'w', address, 4);
        if (ir->rn == 31) {
            stack_pointer += (ir->simm9 << 2);
        } else {
            registers[ir->rn].dword += (ir->simm9 << 2);
        }


    } else if (!strcmp(ir->mnemonic, "stp")) {  // also handles "stnp"
        if (debug) {
            fprintf(logout, "  %s - Rn %#x  Rt %#x  Rt2 %#x, simm7 %#lx\n",
                ir->mnemonic, ir->rn, ir->rt, ir->rt2, ir->simm7);
        }
        int post = 0, pre = 0;
        switch (extract_middle(24, 23, instr)) {
          case 1:   // post-index
            post = 1;
            break;
          case 3:   // pre-index
            pre = 1;
            break;
          case 2:   // signed offset
            break;
          default:
            fprintf(logout, "\nstp: bad bits 24-23 %#x\n",
                extract_middle(24, 23, instr));
        }
        unsigned scale = 2 + (ir->regsize == 64);
        int offset = ir->simm7 << scale;
        unsigned databits = 0x8 << scale;
        unsigned databytes = databits >> 3;
        if (debug)
            fprintf(logout, "scale %#x  offset %#x  databits %#x  databytes %#x\n",
                scale, offset, databits, databytes);

        if (ir->rn == 31) {
            address = stack_pointer;
            if (pre || post)
                stack_pointer += offset;
        } else {
            address = (registers[ir->rn].dword);
            if (pre || post)
                registers[ir->rn].dword += offset;
        }

        if (!post)
            address += offset;

        accessMem(program, registers[ir->rt].bytes, 'w', address, databytes);
        accessMem(program, registers[ir->rt2].bytes, 'w', address + databytes, databytes);


    //---- branches ----

    } else if (!strcmp(ir->mnemonic, "b")) {
        next_program_counter = program_counter + (ir->imm26 << 2);


    } else if (!strcmp(ir->mnemonic, "bl")) {
        long int offset = (ir->imm26 << 2);
        if (debug)
            fprintf(logout, "  opcode:%s  ir->imm26 0x%08lx  offset 0x%08lx / %ld\n",
                ir->mnemonic, ir->imm26, offset, offset);

        registers[30].dword = program_counter + 4;
        next_program_counter = program_counter + offset;

        if (debug)
            fprintf(logout, "  program_counter:0x%08lx  next_program_counter:0x%08lx\n",
                program_counter, next_program_counter);

    } else if (!strcmp(ir->mnemonic, "ret")) {
        next_program_counter = registers[ir->rn].dword;


    } else if (ir->mnemonic[0] == 'b' && ir->mnemonic[1] == '.') {
        /*
        * reference:
        * https://developer.arm.com/documentation/100069/0602/Condition-Codes?lang=en
        */
        unsigned test;
        if (!strcmp(ir->mnemonic+2, "ne"))
            test = (apsr.zero == 0);
        else if (!strcmp(ir->mnemonic+2, "eq"))
            test = (apsr.zero == 1);

        else if (!strcmp(ir->mnemonic+2, "gt"))
            test = ((apsr.zero == 0) && (apsr.overflow == apsr.negative));
        else if (!strcmp(ir->mnemonic+2, "ge"))
            test = (apsr.negative == apsr.overflow);
        else if (!strcmp(ir->mnemonic+2, "lt"))
            test = (apsr.negative != apsr.overflow);
        else if (!strcmp(ir->mnemonic+2, "le"))
            test = (apsr.zero == 1) || (apsr.overflow != apsr.negative);

        else if (!strcmp(ir->mnemonic+2, "hi"))
            test = ((apsr.carry == 1) && (apsr.zero == 0));
        else if (!strcmp(ir->mnemonic+2, "hs"))
            test = (apsr.carry == 1);
        else if (!strcmp(ir->mnemonic+2, "lo"))
            test = (apsr.carry == 0);
        else if (!strcmp(ir->mnemonic+2, "ls"))
            test = (apsr.carry == 0) || (apsr.zero == 1);

        else {
            fprintf(logout, "  Invalid conditional branch %s\n", ir->mnemonic);
        }
        if (debug)
            fprintf(logout, "  Conditional branch %s:  test %d\n", ir->mnemonic, test);

        if (test) {
            if (debug)
                fprintf(logout, "  imm19 %#x\n", (int)ir->imm19<<2);
            long unsigned branch_target;
            branch_target = program_counter + (int)(ir->imm19 << 2);
            next_program_counter = branch_target;
        }


    } else if (!strcmp(ir->mnemonic, "cbz_32")) {
        int branch_target = program_counter + (int)(ir->imm19 << 2);;
        if (debug)
            fprintf(logout, "  branch_target:%#lx\n", next_program_counter);
        if ((size_mask & registers[ir->rt].dword) == 0) {
            next_program_counter = branch_target;
        }

    } else if (!strcmp(ir->mnemonic, "cbnz_32") || !strcmp(ir->mnemonic, "cbnz_64")) {
        int branch_target = program_counter + (int)(ir->imm19 << 2);;
        if (debug) {
            fprintf(logout, "  PC 0x%08lx;  imm19 %#lx\n", program_counter, ir->imm19);
            fprintf(logout, "  branch_target:%#x\n", branch_target);
        }
        if ((size_mask & registers[ir->rt].dword) != 0) {
            next_program_counter = branch_target;
        }


    } else if (!strcmp(ir->mnemonic, "svc")) {
        unsigned length, fd;
        unsigned stroffset;
        unsigned char *strptr;

        switch (registers[8].dword) {
#ifdef ALL
          case 0x3f:    // SYS_read

            fd = registers[0].dword;
            stroffset = (registers[1].dword - program->program_start);
            strptr = ((program->bytes) + stroffset);
            length = registers[2].dword;
            if (debug) {
                fprintf(logout, "  stroffset %#x; strptr %p; length %#x\n",
                    stroffset, strptr, length);
                fflush(NULL);
            }
            unsigned n_read = read(fd, strptr, length);
            strptr[n_read] = '\0';
            registers[0].dword = n_read;
            if (verbose)
                fprintf( logout,
                    "*** begin SYS_read ***\n%u chars read:\n%s\n*** end SYS_read ***\n",
                    n_read, strptr );
            break;
#endif
          case 0x40:    // SYS_write

            fd = registers[0].dword;
            stroffset = (registers[1].dword - program->program_start);
            strptr = ((program->bytes) + stroffset);
            length = registers[2].dword;
            if (debug) {
                fprintf(logout, "  stroffset %#x; strptr %p; length %#x\n",
                    stroffset, strptr, length);
                fflush(NULL);
            }
            write(fd, strptr, length);
            if (verbose)
                fprintf( logout,
                    "*** begin SYS_write ***\n%s\n*** end SYS_write ***\n",
                        strptr );
            break;

          case 0x5d:    // SYS_exit
            fprintf(logout, "SYS_exit\n");
            running = 0;
            break;

          default:
            fprintf(logout, "Unknown service %#lx\n", registers[8].dword);
        }

    } else {
        fprintf(logout, "Unknown instruction %s\n", ir->mnemonic);
    }
    fflush(NULL);   // send all output

    registers[31].dword = 0;    // ensure non-writeable status of xzr
}
//----------------------------------------------------------------
