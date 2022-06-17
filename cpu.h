/* ARMv8 simulation:  define the Datapath
*   Data structures, function prototypes, and global variables that
*   implement a simplistic Arm64 Datapath.
*
* 2022-05-27 v3.0 Implement interactive/batch modes.
* 2022-05-20 v2.1 Cosmetic rearrangement of code, comments added.
* 2022-03-30 v2.0 Move the extern'd variable declarations to memsimulate.c.
* 2021-03-08 v1.0
*/
#ifndef __CPU__
#define __CPU__
#include <stdio.h>
#include <stdlib.h>     // malloc()
#include "memory.h"

// Utilities that extract bitfields from instructions...
#define extract_n_upper(nbits, value) ( (unsigned)(value) >> (32-(nbits)) )
#define extract_n_lower(nbits, value) \
    ( (((unsigned)1 << (unsigned)(nbits)) - (unsigned)1) & (unsigned)(value) )
#define extract_middle(lft, rgt, value) \
    (extract_n_lower(((unsigned)lft+1), (unsigned)(value)) >> (unsigned)(rgt))


// Generic storage for a 32-bit instruction:
union InstructionWord {
    unsigned value;
    unsigned char bytes[4];
};

// struct that holds an instruction and values parsed out of it:
typedef struct Instruction {
    union InstructionWord instruction;
    char *mnemonic; // lookup matching string from the opcode_patterns array

    short unsigned op;  // the opcode
    short unsigned sizebits;
    short unsigned rm, shamt, rn, rd, rt, rt2;  // register numbers (not contents)
    short unsigned lshift, shift;
    short unsigned immr, imms;
    //implicitly promote these to 64 bits:
    long unsigned uimm6, uimm12;
    long int simm7, simm9;
    long int imm16, imm19, imm26;
    // values derived from size bits of the instruction:
    short unsigned regsize;
    long unsigned regsize_mask;
} Instruction;


// An array of these is the Register Bank:
typedef union Register {
    long unsigned dword;
    unsigned word[2];
    unsigned short hword[4];
    unsigned char bytes[8];
} Register;

// Collected status flags:
typedef struct Application_Processor_Status_Register {
    unsigned negative : 1 ;
    unsigned zero     : 1 ;
    unsigned carry    : 1 ;
    unsigned overflow : 1 ;
} APSR;

extern Register registers[];    // CPU core's register bank
extern APSR apsr;               // CPU core's status register

// "stack_pointer" and "program_counter" are actual registers in the CPU.
//  "next_program_counter" is a value that the datapath calculates.
extern long int stack_pointer;
extern long int program_counter, next_program_counter;


// Global storage:
//  All functions have access to these globally-shared variables.
//  They are declared "extern" here for use in any/every file,
//  and declared normally (i.e., storage allocated) with "main()".
extern unsigned running, batch, print, memory_dump, verbose, debug;
extern char *logfile;
extern FILE *logout;


// Miscellaneous function prototypes:

void compile_opcode_regexes(void);  // setup for the fetch-execeute code

void simulate_program(Memory *prog);    // overall fetch-execute loop
void decode(Instruction *ir);
void execute(Instruction *ir, Memory *program);

void displayState(void);            // output function used by main()

#endif
