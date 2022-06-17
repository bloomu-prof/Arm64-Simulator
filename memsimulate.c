/*
* Simulate execution of a program from its memory image.
* 2022-05-27 v3.0 Implement interactive/batch modes.
* 2022-05-21 v2.1 Touch up the comments.
* 2022-03-30 v2.0 Move some global variable declarations from "cpu." to here;
*   they're extern'd in the other files.
* 2021-03-08 v1.0
*/
#include <stdio.h>
#include <string.h>     // strlen()
#include "cpu.h"        // global flags, fetch_decode_execute()

//--------------------------------
// This stuff is moved from "cpu.h" ---
Register registers[32];
APSR apsr;
unsigned running, batch, print, memory_dump, verbose, debug;
char *logfile;
FILE *logout;

long int stack_pointer;
long int program_counter, next_program_counter;
//--------------------------------

void help(char *s)
{
    char helpmsg[] =
        "usage: %s [option ...] [-l logfile] <filename>\n"
        "       -h    Help\n"
        "       -l <filename>   simulator output to <filename>\n"
        "       -m    Memory-dump to file\n"
        "       -p    Print memory load\n"
        "       -D    Debug\n"
    ;
    fprintf(stderr, helpmsg, s);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        help(argv[0]);
        return 1;
    }

    Memory progMemory;  // struct containing the array of "unsigned char" bytes.

    // Parse the command line options:
    print = memory_dump = debug = 0;  // global flags
    logfile = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp("-h", argv[i])) {
            help(argv[0]);
            return 1;
        } else if (!strcmp("-l", argv[i])) {
            logfile = argv[i+1];
        } else if (!strcmp("-m", argv[i])) {
            memory_dump = 1;
        } else if (!strcmp("-p", argv[i])) {
            print = 1;
        } else if (!strcmp("-D", argv[i])) {
            debug = 1;
        }
    }

    if (logfile) {
        logout = fopen(logfile, "w");
    } else {
        logout = stderr;
    }

    /*--------------------------------
    * Open the executable file, read it,
    *   and fill the Memory object with the contents:
    */
    fillmem(&progMemory, argv[argc-1]);

    fprintf(logout, "%d memory/instruction bytes (%#x)\n",
        progMemory.nbytes, progMemory.nbytes);

    //--------------------------------
    // Display the loaded memory bytes:
    if (print == 1)
        display_memory(&progMemory);

    //--------------------------------
    // Dump the pre-execution memory, for comparison:
    if (memory_dump) {
        // No real need to write the memory out to a disk file,
        // but it's informative:
        FILE *m = fopen("memory-begin.dump", "wb");
        fwrite(progMemory.bytes, 1, progMemory.nbytes, m);
        fclose(m);
    }

    //--------------------------------
    // Run the program, simulating an ARMv8 processor running Linux:
    simulate_program(&progMemory);

    /*
    * Finish things up.
    */

    fclose(logout);

    //--------------------------------
    // Dump the post-execution memory, for comparison:
    if (memory_dump) {
        FILE *m = fopen("memory-end.dump", "wb");
        fwrite(progMemory.bytes, 1, progMemory.nbytes, m);
        fclose(m);
    }

    return 0;
}
//-----------------------------------------------------------------------
