/*
* decode instruction words
* 2022-05-27 v3.0 Implement interactive/batch modes (no effect on this file).
* 2021-03-02 v1.0
*/
#include <stdio.h>
#include <string.h>     // strlen(), strcmp()
#include <regex.h>
#include "cpu.h"
#include "opcode_patterns.h"

#define MAX_ERROR_MSG 0x1000

// A node for the linked list of opcode regular expressions:
typedef struct OpcodeRegexes {
    char *pattern;
    char *mnemonic;
    regex_t re;
    struct OpcodeRegexes *next;
} OpcodeRegexes;

static OpcodeRegexes *opcode_regexes;

/*
* One-time build the instruction patterns into a linked list of regexes,
*   for comparison against (stringified) instruction words.
*/
void compile_opcode_regexes(void)
{
    OpcodeRegexes *start = NULL, *end = NULL;
    regex_t re;
    for (int i = 0; i < n_opcode_patterns; i++) {
        int status = regcomp(&re, opcode_patterns[i].pattern, REG_EXTENDED);
        if (status == 0) {
            OpcodeRegexes *r = malloc(sizeof(struct OpcodeRegexes));
            r->pattern = opcode_patterns[i].pattern;
            r->mnemonic = opcode_patterns[i].mnemonic;
            r->re = re;
            r->next = NULL;
            // append this node to the list:
            if (start == NULL)
                start = r;
            else
                end->next = r;
            end = r;
        } else {
            char error_message[MAX_ERROR_MSG];
            regerror(status, &re, error_message, MAX_ERROR_MSG);
            fprintf(logout, "Regex error compiling '%s': %s\n",
                     opcode_patterns[i].pattern, error_message);
        }
    }
    opcode_regexes = start;
}
//----------------------------------------------------------------


// Accept a bitstring and flip it end-for-end.
void reverse_in_place(char *s)
{
    char *p = s;
    while (*(p++) != '\0')
        ;
    p--;
    do {
        char t = *(--p);
        *p = *s;
        *(s++) = t;
    } while (p > s);
}
//--------

// Convert a 32-bit instruction to an ASCII bitstring.  The result
//  is reversed, so use "reverse_in_place()" to correct that.
void to_bitstring(char *b, unsigned v, unsigned nbits, char sep)
{
    char *p;
    int i;
    for (p = b, i = 0; i < nbits; i++) {
        *(p++) = (v & 0x01) + '0';
        if ( (sep != 0) && (3 == (i % 4)) )
            *(p++) = sep;
        v >>= 1;
    }
    if (*(p-1) == sep)
        p--;
    *p = '\0';
    reverse_in_place(b);
}
//--------

// Convert the instruction from a binary value to an ASCII bitstring, then use
//  regular expressions to match the bitstring to one of the patterns found in
//  the opcode_regexes list.
//  This match sets a mnemonic into the Instruction struct.  The mnemonic is
//  useful for the simulator's output, but in this version of the simulator
//  it is also used by "execute()" to determine what to do.  Ugh?
#define NMATCH 30
void set_mnemonic(Instruction *instr)
{
    regmatch_t pmatch[NMATCH];
    char bitstring_bfr[64];
    to_bitstring(bitstring_bfr, instr->instruction.value, 32, 0);
    for (OpcodeRegexes *p = opcode_regexes; p != NULL; p = p->next) {
        if (!regexec(&(p->re), bitstring_bfr, NMATCH, pmatch, 0)) {
            instr->mnemonic = p->mnemonic;
            if (debug)
                fprintf(logout, "\n  set_mnemonic(): Matched: %s\n", instr->mnemonic);
            return;
        }
    }
    to_bitstring(bitstring_bfr, instr->instruction.value, 32, '_');
    fprintf(logout, "  set_mnemonic(): No match for instruction 0x%08x / %s\n",
        instr->instruction.value, bitstring_bfr);
}
//--------

// One of the control-logic functions of an Arm64 CPU.
long int sign_extend(unsigned value, unsigned nbits, unsigned new_nbits)
{
    long int new_value = value;
    long unsigned signbit = (value >> (nbits - 1)) << nbits;
    for (int i = nbits; i < new_nbits; i++) {
        new_value |= signbit;
        signbit <<= 1;
    }
    return new_value;
}
//--------


/*
* Extract various control signals from the instruction's bits.
*  Also add a corresponding mnemonic.
*/
void decode(Instruction *ir)
{
    char display_bfr[40];
    unsigned v = ir->instruction.value;
    to_bitstring(display_bfr, v, 32, '_');
    if (verbose)
        fprintf(logout, "Decode - instruction bitstring:%s\n", display_bfr);

    ir->rm = extract_middle(20, 16, v);
    ir->rn = extract_middle(9, 5, v);
    ir->rt = ir->rd = extract_n_lower(5, v);
    ir->rt2 = extract_middle(14, 10, v);    // aka ra for "madd"

    if (verbose) {
        fprintf(logout,
            "    (ir.rm: 0x%02x)  (ir.rn: 0x%02x)  (ir.rd/rt: 0x%02x)  (ir.rt2: 0x%02x)\n",
            ir->rm, ir->rn, ir->rd, ir->rt2);
        fflush(NULL);
    }

    ir->shamt = extract_middle(15, 10, v);
    ir->lshift = extract_middle(22, 22, v);
    ir->shift = extract_middle(23, 22, v);
    if (debug) {
        fprintf(logout, "decode - lshift %#x  shamt %#x\n", ir->lshift, ir->shamt);
    }
    //ir->regsize = ( (0x80000000 & ir->instruction.value) ? 64 : 32 );
    ir->sizebits = extract_n_upper(2, v);
    if (0x80000000 & ir->instruction.value) {
        ir->regsize = 64;
        ir->regsize_mask = 0xffffffffffffffff;
    } else {
        ir->regsize = 32;
        ir->regsize_mask = 0x00000000ffffffff;
    }

    if (debug)
        fprintf(logout, "decode - rm:%#04x; shamt:%#x; rn:%#04x, rt/rd:%#04x\n",
            ir->rm, ir->shamt, ir->rn, ir->rd );

    ir->immr = extract_middle(21, 16, v);
    ir->imms = extract_middle(15, 10, v);
    ir->uimm6 = sign_extend( extract_middle(15, 10, v), (15-10+1), 64 );
    ir->simm7 = sign_extend( extract_middle(21, 15, v), (21-15+1), 64 );
    ir->simm9 = sign_extend( extract_middle(20, 12, v), (20-12+1), 64 );
    ir->uimm12 = (long unsigned)extract_middle(21, 10, v);
    ir->imm16 = extract_middle(20, 5, v);
    ir->imm19 = sign_extend( extract_middle(23, 5, v), (23-5+1), 64 );
    ir->imm26 = sign_extend( extract_n_lower(26, v), 26, 64 );

    if (debug) {
        fprintf(logout, "  uimm6 %#lx; uimm12 %#lx; simm7 %#lx\n",
            ir->uimm6, ir->uimm12, ir->simm7);
        fprintf(logout,
            "  simm9 %#lx; imm16 %#lx; imm19 %#lx; imm26 0%#lx\n",
            ir->simm9, ir->imm16, ir->imm19, ir->imm26);
        fflush(NULL);
    }

    set_mnemonic(ir);
}
//--------
