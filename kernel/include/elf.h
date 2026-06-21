#ifndef ELF_H
#define ELF_H

#include <kernel/paging.h>
#include <stdint.h>
#include <types.h>

typedef struct
{
    u32 type;
    u32 flags;

    ptr_t virtual_addr;
    ptr_t physical_addr;

    ptr_t file_offset;

    ptr_t size_file;
    ptr_t size_memory;

    ptr_t align;
} elf_program_header;

typedef struct
{
    int fd;
    size_t program_headers_len;
    ptr_t entry_point;
    elf_program_header **program_headers;
} elf;

elf *parse_elf32(int fd);
elf *parse_elf64(int fd);
elf *parse_elf(int fd);
struct task *load_elf(elf *elf, int argc, char **argv);
void free_elf(elf *elf);

#endif
