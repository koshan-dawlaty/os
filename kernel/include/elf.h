#ifndef ELF_H
#define ELF_H

#include <stdint.h>

typedef struct
{

    uint32_t type;
    uint32_t file_offset;
    uint32_t virutal_addr;
    uint32_t physical_addr;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
    uint32_t align;
} elf_program_header_32;

typedef struct
{
    uint16_t program_headers_len;
    elf_program_header_32 **program_headers;
    uint32_t entry_point;

    int fd;
} elf_32;

elf_32 *parse_elf32(int fd);
void load_elf32(elf_32 *elf);

#endif
