#ifndef ELF_H
#define ELF_H

#include <kernel/paging.h>
#include <stdint.h>
#include <types.h>

typedef struct
{
    u32 type;
    u32 flags;

    ptr virtual_addr;
    ptr physical_addr;

    ptr file_offset;

    ptr size_file;
    ptr size_memory;

    ptr align;
} elf_program_header;

typedef struct
{
    int fd;
    size_t program_headers_len;
    ptr entry_point;
    elf_program_header **program_headers;
} elf;

elf *parse_elf32(int fd);
elf *parse_elf64(int fd);
elf *parse_elf(int fd);
struct task *load_elf(elf *elf, int argc, char **argv);
void free_elf(elf *elf);

#endif
