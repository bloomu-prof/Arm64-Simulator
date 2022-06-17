// Implementation for the memory data structure.
//  This file includes the functions needed to fill, and access, main memory.
// 2022-05-27 v3.0 Implement interactive/batch modes.
#include <string.h>     // strcmp()
#include <elf.h>
#include "memory.h"
#include "cpu.h"        // global flags

#define roundup(v, bits)    (( ((v) + ((1<<(bits)) - 1)) >> (bits) )<<(bits))

/*
* Utility functions to extract info from the executable file's sections:
*   report_section() - display info
*   section_name() - extract a section's name
*   section_index() - return a section's location within the executable file.
*/
void report_section(char *name, int index,
    long unsigned addr, long unsigned size, unsigned offset, unsigned end)
{
    fprintf(logout, "  %s section_index = %#x\n", name, index);
    fprintf(logout, "      sh_addr = %#lx  sh_size = %#lx\n", addr, size);
    fprintf(logout, "      offset = %#x  end = %#x\n", offset, end);
}
//--------------------------------

char *section_name(
    Elf64_Shdr *section_header_table,
    char *strings_section,
    int index)
{
    Elf64_Shdr shdr = section_header_table[index];
    return (strings_section + shdr.sh_name);
}
//--------------------------------

int section_index(FILE *h, char *section_name)
{
    Elf64_Ehdr elf_hdr;
    fseek(h, 0, SEEK_SET);
    fread(&elf_hdr, sizeof(elf_hdr), 1, h);

    int section_hdr_tbl_size = elf_hdr.e_shentsize * elf_hdr.e_shnum;
    Elf64_Shdr section_header_table[section_hdr_tbl_size];
    fseek(h, elf_hdr.e_shoff, SEEK_SET);
    fread(section_header_table, elf_hdr.e_shentsize, elf_hdr.e_shnum, h);

    Elf64_Shdr *strings_section_hdr = section_header_table + elf_hdr.e_shstrndx;
    char strings_section[strings_section_hdr->sh_size];
    fseek(h, strings_section_hdr->sh_offset, SEEK_SET);
    fread(strings_section, 1, strings_section_hdr->sh_size, h);

    // search for a section, return its offset
    for (int i = 0; i < elf_hdr.e_shnum; i++) {
        Elf64_Shdr shdr = section_header_table[i];
        if (!strcmp((char *)(strings_section + shdr.sh_name), section_name)) {
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------


// Convert a program virtual address to an array index,
//  then access memory bytes starting at that address (index).
void accessMem(
    Memory *progMemory, unsigned char *memBus, char rw,
    long unsigned addr, unsigned nbytes)
{
    long unsigned addr_array = addr - progMemory->program_start;
    if (verbose)
        fprintf( logout,
            "    accessMem() %c - requested addr %#lx, array addr %#lx\n",
            rw, addr, addr_array );

    if (rw == 'w')
        for (int i = 0; i < nbytes; i++)
            progMemory->bytes[addr_array + i] = memBus[i];
    else if (rw == 'r')
        for (int i = 0; i < nbytes; i++)
            memBus[i] = progMemory->bytes[addr_array + i];
    else {
        fprintf(logout, "!!! accessMem() - bad 'rw' value!\n");
    }
}
//----------------------------------------------------------------


// display_memory() - print out the memory contents.
void display_memory(Memory *progMemory)
{
    int prtline = 1;
    fprintf(logout, "#--------------------------------\n");
    for (int i = 0; i < progMemory->nbytes; i += 4) {
        if ( (i >= progMemory->nbytes - 4)
            || (progMemory->bytes[i+0] != 0x00)
            || (progMemory->bytes[i+1] != 0x00)
            || (progMemory->bytes[i+2] != 0x00)
            || (progMemory->bytes[i+3] != 0x00)
        ) {
            fprintf(logout, "  0x%02x  ", i);
            for (int j = 3; j >= 0; j--)
                fprintf(logout, " %02x", progMemory->bytes[i+j]);
            fprintf(logout, "\n");
            prtline = 1;
        } else if (prtline == 1) {
            fprintf(logout, "  0x%02x     ...\n", i);
            prtline = 0;
        }
    }
    fprintf(logout, "#--------------------------------\n");
}
//-----------------------------------------------------------------------


// fillmem() - primary function for reading an executable file

void fillmem(Memory *progMemory, char *filename)
{
    unsigned section_end;
    FILE *h = fopen(filename, "rb");

    fprintf(logout, "fillmem():\n");

    // Read ELF Header
    Elf64_Ehdr elf_hdr;
    fread(&elf_hdr, sizeof(elf_hdr), 1, h);

    if (verbose) {
        fprintf(logout, "  elf_hdr.e_phentsize %#x, elf_hdr.e_phnum %#x\n",
            elf_hdr.e_phentsize, elf_hdr.e_phnum);
        fprintf(logout, "  elf_hdr.e_shentsize %#x, elf_hdr.e_shnum %#x\n",
            elf_hdr.e_shentsize, elf_hdr.e_shnum);
        fprintf(logout, "  elf_hdr.e_entry %#lx\n", elf_hdr.e_entry);
    }

    // Read Program Header
    int program_hdr_tbl_size = elf_hdr.e_phentsize * elf_hdr.e_phnum;
    Elf64_Phdr program_header_table[ program_hdr_tbl_size ];

    fseek(h, elf_hdr.e_phoff, SEEK_SET);
    fread(&program_header_table, elf_hdr.e_phentsize, elf_hdr.e_phnum, h);

    if (verbose) {
        fprintf(logout, "  entry  p_offset    p_vaddr   p_memsz   :program header\n");
        for (int i = 0; i < elf_hdr.e_phnum; i++) {
            if (program_header_table[i].p_type != 0x00)
                fprintf(logout,
                    "    %2d    %#6lx  %#10lx   %#6lx\n",
                    i,
                    program_header_table[i].p_offset,
                    program_header_table[i].p_vaddr,
                    program_header_table[i].p_memsz);
        }
        fprintf(logout, "\n");
    }

    // Read Section Headers
    int section_hdr_tbl_size = elf_hdr.e_shentsize * elf_hdr.e_shnum;
    Elf64_Shdr section_header_table[ section_hdr_tbl_size ];
    //Elf64_Shdr strings_section_hdr = section_header_table[elf_hdr.e_shstrndx];
    //char strings_section[strings_section_hdr.sh_size];

    fseek(h, elf_hdr.e_shoff, SEEK_SET);
    fread(&section_header_table, elf_hdr.e_shentsize, elf_hdr.e_shnum, h);

    Elf64_Shdr strings_section_hdr = section_header_table[elf_hdr.e_shstrndx];
    char strings_section[strings_section_hdr.sh_size];
    fseek(h, strings_section_hdr.sh_offset, SEEK_SET);
    fread(strings_section, 1, strings_section_hdr.sh_size, h);

    if (verbose) {
        fprintf(logout, "  section        name  sh_offset   sh_addr  sh_size\n");
        for (int i = 0; i < elf_hdr.e_shnum; i++) {
            if (section_header_table[i].sh_type == 0x00)
                continue;
            char *secname = section_name( section_header_table, strings_section, i);
            fprintf(logout,
                "    %2d %14s   %#6lx    %#8lx   %#6lx\n",
                i, secname, section_header_table[i].sh_offset,
                section_header_table[i].sh_addr, section_header_table[i].sh_size);
        }
        fprintf(logout, "\n");
    }

    progMemory->entry = elf_hdr.e_entry;    // virtual execution entry

    // Starting address for loading code into actual memory
    progMemory->program_start = program_header_table[0].p_vaddr;

    progMemory->nbytes = 0;
    fprintf(logout, "  nbytes: %#x\n", progMemory->nbytes);

    Elf64_Shdr text_section_hdr;
    int text_index = section_index(h, ".text");
    if (text_index >= 0) {
        text_section_hdr = section_header_table[text_index];
        progMemory->text_offset =
            text_section_hdr.sh_addr - progMemory->program_start;
        section_end =
            progMemory->text_offset + roundup(text_section_hdr.sh_size, 2);
        report_section(".text",
            text_index,
            text_section_hdr.sh_addr,
            text_section_hdr.sh_size,
            progMemory->text_offset, section_end);
        if (section_end > progMemory->nbytes) {
            progMemory->nbytes = section_end;
            fprintf(logout, "  .text nbytes: %#x\n", progMemory->nbytes);
        }
    } else {
        progMemory->text_offset = -1;
        fprintf(logout, "  .text index %#x, offset %#lx\n",
            text_index, progMemory->text_offset);
    }
    fprintf(logout, "\n");

    Elf64_Shdr data_section_hdr;
    int data_index = section_index(h, ".data");
    if (data_index > 0) {
        data_section_hdr = section_header_table[data_index];
        progMemory->data_offset =
            data_section_hdr.sh_addr - progMemory->program_start;
        section_end =
            progMemory->data_offset + roundup(data_section_hdr.sh_size, 2);
        report_section(".data", data_index,
            data_section_hdr.sh_addr, data_section_hdr.sh_size,
            progMemory->data_offset, section_end
        );
        if (section_end > progMemory->nbytes) {
            progMemory->nbytes = section_end;
            fprintf(logout, "  .data nbytes: %#x\n", progMemory->nbytes);
        }
    } else {
        progMemory->data_offset = -1;
        fprintf(logout, "  .data index %#x, offset %#lx\n",
            data_index, progMemory->data_offset);
    }
    fprintf(logout, "\n");

    Elf64_Shdr bss_section_hdr;
    int bss_index = section_index(h, ".bss");
    if (bss_index > 0) {
        bss_section_hdr = section_header_table[bss_index];
        progMemory->bss_offset =
            bss_section_hdr.sh_addr - progMemory->program_start;
        section_end =
            progMemory->bss_offset + roundup(bss_section_hdr.sh_size, 2);
        report_section(".bss", bss_index,
            bss_section_hdr.sh_addr, bss_section_hdr.sh_size,
            progMemory->bss_offset, section_end
        );
        if (section_end > progMemory->nbytes) {
            progMemory->nbytes = section_end;
            fprintf(logout, "  .bss nbytes: %#x\n", progMemory->nbytes);
        }
    } else {
        progMemory->bss_offset = -1;
        fprintf(logout, "  .bss index %#x, offset %#lx\n",
            bss_index, progMemory->bss_offset);
    }
    fprintf(logout, "\n");

    fflush(NULL);

    //----------------
    progMemory->text_start = text_section_hdr.sh_addr - progMemory->program_start;
    progMemory->data_start = data_section_hdr.sh_addr - progMemory->program_start;
    progMemory->bss_start = bss_section_hdr.sh_addr - progMemory->program_start;

    progMemory->nbytes += STACKSIZE;
    fprintf(logout, "  nbytes: %#x\n", progMemory->nbytes);

    // Allocate space for the text segment and whatever should come before it:
    progMemory->bytes = malloc(progMemory->nbytes);

    // virtual text-segment offset:
    if (text_index > 0) {
        fseek(h, text_section_hdr.sh_offset, SEEK_SET);
        fread(progMemory->bytes + progMemory->text_start,
            1, text_section_hdr.sh_size, h);
    }

    // virtual data-segment offset:
    if (data_index > 0) {
        fseek(h, data_section_hdr.sh_offset, SEEK_SET);
        fread(progMemory->bytes + progMemory->data_start,
            1, data_section_hdr.sh_size, h);
    }

    fclose(h);

    fprintf(logout, "\n  progMemory->bytes:%p\n", progMemory->bytes);
    fprintf(logout, "  progMemory->nbytes:%#x\n", progMemory->nbytes);
    fprintf(logout, "  progMemory->entry:%#010lx\n", progMemory->entry);
    fprintf(logout, "  progMemory->text_start:%#010lx\n",
        progMemory->text_start);
    fprintf(logout, "  relative entry:%#lx\n",
        (progMemory->entry - progMemory->program_start));
    fprintf(logout, "fillmem() done.\n\n");
}
//-----------------------------------------------------------------------
