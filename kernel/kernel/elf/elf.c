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

void free_elf(elf *elf)
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

elf *parse_elf(int fd)
{
#ifdef CONFIG_x86_32
    return parse_elf32(fd);
#endif
    return parse_elf64(fd);
}

struct task *load_elf(elf *elf, int argc, char **argv)
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

    printf("loading pgdir\n");

    load_pgdir(pgdir);

    printf("pagedir is now invalid\n");

    for (size_t j = 0; j < elf->program_headers_len; j++)
    {
        elf_program_header *header = elf->program_headers[j];
        /* load */
        if (header->type == 1)
        {
            for (size_t i = 0; i <= header->size_memory + PAGE_SIZE;
                 i += PAGE_SIZE)
            {

                map_page((void *)get_free_phys_page(),
                         (void *)(header->virtual_addr + i),
                         PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
            }

            /* this may not have zeros at the end */
            uint8_t *buf = malloc(header->size_memory);
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

            pread(elf->fd, buf, header->size_file, header->file_offset);

            uint8_t *dest = (uint8_t *)(header->virtual_addr);

            for (size_t i = 0; i < header->size_memory; i++)
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

    printf("test\n");
    load_pgdir(temp_pgdir);

    return t;
}
