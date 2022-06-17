/* aarch64 simulation - memory specification
* 2022-05-21
*/
#ifndef __MEMORY__
#define __MEMORY__
#include <stdio.h>      // FILE *

#define BASE_ADDR_TEXT 0x400000    // Find this in the ELF program header instead?
#define BASE_ADDR_DATA 0x410000    // Find this in the ELF program header instead?
#define STACKSIZE 1024  // space for 128 registers' worth

/*
* This data structure holds the various segment-offset locations that are extracted
*   from the executable file, along with a dynamic array that holds the actual text,
*   data, bss, stack, and heap space.
*/
typedef struct {
    long unsigned program_start;    // where the program begins in memory
    long unsigned entry;            // execution virtual starting-point

    long unsigned text_start;       // where the text would load
    long int text_offset;           // loading address for text segment

    long unsigned data_start;       // where the data would load
    long int data_offset;           // loading address for data segment

    long unsigned bss_start;        // where the data would load
    long int bss_offset;            // loading address for data segment

    unsigned char *bytes;           // the actual memory contents
    unsigned nbytes;                // total progam size
} Memory;

// Function prototypes for working with the memory struct:
void display_memory(Memory *progMemory);
void fillmem(Memory *progMemory, char *filename);
void accessMem(
    Memory *progMemory, unsigned char *memBus, char rw,
    long unsigned addr, unsigned nbytes);

#endif
