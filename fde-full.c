/*
* Simulate an arm64 processor's Fetch-Execute cycle.
* 2022-05-27 v3.0 Make it interactive with a "REPL".
* 2022-05-22 v2.0 Clean up "apsr" warning.
* 2021-03-02
*/
#include <stdio.h>
#include "cpu.h"
#include "memory.h"

/*
* Utility function to display register values, status register, pc & sp
*/
void displayState(void)
{
    fprintf(logout, "#--------\n");
    for (unsigned i = 0; i < 11; i++) {
        fprintf(logout, "  X%02u:0x%016lx", i, registers[i].dword);
        if (i+11 < 32)
            fprintf(logout, " X%02u:0x%016lx", i+11, registers[i+11].dword);
        if (i+22 < 32)
            fprintf(logout, " X%02u:0x%016lx", i+22, registers[i+22].dword);
        fprintf(logout,"\n");
    }
    fprintf(logout, "  negative:%u  zero:%u  carry:%u  overflow:%u\n",
        apsr.negative, apsr.zero, apsr.carry, apsr.overflow);
    fprintf(logout, "  program_counter:0x%08lx    stack_pointer:0x%08lx\n",
        program_counter, stack_pointer);
    fprintf(logout, "#--------\n");
    fflush(NULL);
}
//----------------------------------------------------------------

/*
* One Fetch-Decode-Execute cycle.
*   Fetch is done by calling the "accessMem()" function from "memory.h".
*   Decode is abstracted into "decode()".
*   Execute is handled by "execute()", and the program counter is updated here.
*/
void one_fde_cycle(Memory *progMemory)
{
    Instruction ir;

    if (program_counter - progMemory->program_start >= progMemory->nbytes) {
        fprintf(logout, "Program Counter exceeds memory size!\n");
        fflush(NULL);
        running = 0;
    }

    //----------------
    // Fetch:
    // Despite superficial appearances, "ir.instruction.bytes" is a pointer:
    if (verbose)
        fprintf(logout, "Fetch - PC %#lx\n", program_counter);

    accessMem(
        progMemory, (unsigned char *)ir.instruction.bytes, 'r',
        program_counter, 4 );

    if (verbose) {
        fprintf(logout, "    fetched %08x (", ir.instruction.value);
        for (int i = 0; i < 4; i++)
            fprintf(logout, " %02x", ir.instruction.bytes[i]);
        fprintf(logout, " )\n");
        fflush(NULL);
    }

    //----------------
    // Decode:
    decode(&ir);

    //----------------
    // Execute:
    if (verbose)
        fprintf(logout, "Execute - %s\n", ir.mnemonic);
    fflush(NULL);

    next_program_counter = program_counter + 4; // default to next instruction
                                // this may change "next_program_counter",
    execute(&ir, progMemory);   // not to mention "running", the registers, etc.

    program_counter = next_program_counter;
}
//----------------------------------------------------------------

/*
* Do a "read-eval-print" loop --- each pass through the loop gets a command
*   from the keyboard and does whatever is asked for.
*/
void simulate_program(Memory *progMemory)
{
    fprintf(logout, "Fetch-Decode-Execute:\n");

    compile_opcode_regexes();   // build the regular expressions needed...
                                // ...to match and identify instructions.

    // Initialize the global status register:
    apsr.negative = 0;
    apsr.zero = 0;
    apsr.carry = 0;
    apsr.overflow = 0;

    // Initialize PC and SP:
    stack_pointer = progMemory->program_start + progMemory->nbytes;
    program_counter = progMemory->entry;
    fprintf(logout,
        "(initial array-index) initial program_counter %#08lx  stack_pointer %#08lx\n",
        program_counter, stack_pointer);

    /*
    * A-a-a-nd here we go!
    *   This "read-eval-print" loop performs one user command.
    *   If the command is "step" (or <Enter> without a command),
    *       simulate the next source instruction.
    *   If the command is "run",
    *       the source instructions are simulated one after
    *       another without accepting any more user input.
    *   It keeps looping until some event changes the value of "running";
    *   for example, executing the SYS_exit supervisor call (see "execute()").
    */
    running = 1;
    batch = 0;
    verbose = 0;
    while (running) {
        if (batch) {
            one_fde_cycle(progMemory);  // just keep simulatin'

        } else {
            // user prompt:
            printf("\nPC:0x%08lx  Command [hsiSprqv] or <Enter> : ", program_counter);

            char *kbd_input = NULL;
            size_t kbd_n;
            getline(&kbd_input, &kbd_n, stdin);
            switch (kbd_input[0]) {
            case 0x0a:
            case 's':   // step
                one_fde_cycle(progMemory);
                break;

            case 'i':   // show register values
                displayState();
                break;

            case 'S':   // Step'n'display
                one_fde_cycle(progMemory);
                displayState();
                break;

            case 'p':   // show program memory
                display_memory(progMemory);
                break;

            case 'v':   // toggle the "verbose" flag
                verbose = ~verbose;
                printf("verbose: %u\n", verbose & 0x01);
                break;

            case 'r':   // switch to batch mode
                batch = 1;
                break;

            case 'q':   // abandon the program
                running = 0;
                break;

            default:
                printf(
                    "h - Help (this output)\n"
                    "s - Step through the next instruction\n"
                    "(no command) - step through the next instruction\n"
                    "i - (Information) display the register values (\"state\")\n"
                    "S - step through next instruction and Show the resulting state\n"
                    "p - Print the program memory\n"
                    "v - toggle the Verbose flag\n"
                    "r - Run the program in 'batch' mode\n"
                    "q - Quit the program\n"
                );
            }
            free(kbd_input);
        }
    }
}
//----------------------------------------------------------------
