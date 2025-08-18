#include "gdt.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"
#include "scheduler.h"
#include "sys/types.h"
#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* TODO: delete this */
extern struct pagedir init_pgdir;
uint32_t word_from_buffer(uint8_t *buf)
{
    return bytes_to_32_bit_int(buf[0], buf[1], buf[2], buf[3]);
}
uint16_t short_from_buffer(uint8_t *buf)
{
    return bytes_to_16_bit_int(buf[0], buf[1]);
}
elf_32 *parse_elf32(int fd)
{
    uint8_t *header_buf = malloc(52);
    pread(fd, header_buf, 52, 0);
    if (header_buf[0] != 0x7F || header_buf[1] != 'E' || header_buf[2] != 'L' ||
        header_buf[3] != 'F')
    {
        printf("invalid elf header\n");
        return NULL;
    }
    elf_32 *out = malloc(sizeof(elf_32));
    uint32_t program_header_table_offset = word_from_buffer(header_buf + 28);
    uint32_t program_header_size = short_from_buffer(header_buf + 42);

    uint32_t section_header_table_offset = word_from_buffer(header_buf + 32);
    uint32_t section_header_size = short_from_buffer(header_buf + 46);

    out->program_headers_len = short_from_buffer(header_buf + 44);
    out->section_headers_len = short_from_buffer(header_buf + 48);
    out->entry_point = word_from_buffer(header_buf + 24);

    uint8_t *program_headers_buf =
        malloc(program_header_size * out->program_headers_len);

    pread(fd, program_headers_buf,
          program_header_size * out->program_headers_len,
          program_header_table_offset);

    elf_program_header_32 **program_headers =
        malloc(sizeof(elf_program_header_32 *) * out->program_headers_len);
    for (size_t i = 0; i < out->program_headers_len; i++)
    {
        elf_program_header_32 *new_header =
            malloc(sizeof(elf_program_header_32));
        memcpy(new_header, (program_headers_buf) + (i * program_header_size),
               sizeof(elf_program_header_32));

        program_headers[i] = new_header;
    }
    out->program_headers = program_headers;

    free(program_headers_buf);

    /* section headers seem to not be needed */
    /*
uint8_t *section_headers_buf =
    malloc(section_header_size * out->section_headers_len);
pread(fd, section_headers_buf,
      section_header_size * out->section_headers_len,
      section_header_table_offset);
elf_section_header_32 **section_headers =
malloc(sizeof(elf_section_header_32 *) * out->section_headers_len);

for (size_t i = 0; i < out->section_headers_len; i++)
{
    elf_section_header_32 *new_header =
        malloc(sizeof(elf_section_header_32));
memcpy(new_header, (section_headers_buf) + (i * section_header_size),

           sizeof(elf_section_header_32));
    section_headers[i] = new_header;
}
out->section_headers = section_headers;
    */

    out->fd = fd;

    free(header_buf);

    return out;
}

void free_elf32(elf_32 *elf)
{
    for (size_t i = 0; i < elf->program_headers_len; i++)
    {
        free(elf->program_headers[i]);
    }
    free(elf);
}

/* copies args to userspace */
char **create_user_args(int argc, char **argv)
{
    /* TODO: this can run out of space */
    char **args = get_free_pages_user(1);
    size_t string_offset = argc * sizeof(char *);

    for (int i = 0; i < argc; i++)
    {
        memcpy(args + string_offset, argv[i], strlen(argv[i]));
        *(char *)(args + string_offset + strlen(argv[i]) + 1) = 0;

        args[i] = (char *)(args + string_offset);

        string_offset += strlen(argv[i]) + 1;
    }

    return args;
}

struct task *load_elf32(elf_32 *elf, int argc, char **argv)
{

    struct task *t = new_task(elf->entry_point, 0xBFFEF000);
    struct pagedir *pgdir = t->pgdir;

    struct pagedir *temp_pgdir = get_current_pagedir();

    char **args_temp = malloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++)
    {
        args_temp[i] = malloc(strlen(argv[i]) + 1);
        memcpy(args_temp[i], argv[i], strlen(argv[i]) + 1);
        args_temp[i][strlen(argv[i])] = 0;
    }

    load_pgdir(pgdir);

    for (size_t j = 0; j < elf->program_headers_len; j++)
    {
        elf_program_header_32 *header = elf->program_headers[j];
        /* load */
        if (header->type == 1)
        {
            for (size_t i = 0; i <= header->memory_size + PAGE_SIZE;
                 i += PAGE_SIZE)
            {

                map_page((void *)get_free_phys_page(),
                         (void *)(header->virutal_addr + i),
                         PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
            }

            /* this may not have zeros at the end */
            uint8_t *buf = malloc(header->memory_size);
            /*
if (header->memory_size > header->file_size)
{

    for (size_t i = 0; i < header->memory_size - header->file_size;
         i++)
    {
        buf[header->file_size + i] = 0;
    }
}
            */

            pread(elf->fd, buf, header->file_size, header->file_offset);

            uint8_t *dest = (uint8_t *)(header->virutal_addr);

            for (size_t i = 0; i < header->memory_size; i++)
            {

                dest[i] = buf[i];
            }
        }
        else
        {
            printf("unrecognized header type\n");
        }
    }

    create_userspace_stack(pgdir);

    char **args = create_user_args(argc, args_temp);

    *(char ***)(0xBFFEF000 + 4) = args;
    *(int *)(0xBFFEF000) = argc;

    load_pgdir(temp_pgdir);

    return t;
}
