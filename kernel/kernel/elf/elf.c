#include "gdt.h"
#include "kernel/addr_space.h"
#include "kernel/paging.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"
#include "panic.h"
#include "scheduler.h"
#include "strings.h"
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

elf *parse_elf(int fd)
{
#ifdef CONFIG_x86_32
    return parse_elf32(fd);
#endif
    return parse_elf64(fd);
}

struct task *load_elf(elf *elf, int argc, char **argv)
{

    struct task *t = new_task_user(elf->entry_point, 0xBFFEF000, argc, argv);
    struct pagedir *pgdir = t->pgdir;
    t->addr_space = (struct addr_space){.dir = t->pgdir, .list_head = NULL};
    struct pagedir *temp_pgdir = get_current_pagedir();

    addr_space_load(t->addr_space);
    for (size_t j = 0; j < elf->program_headers_len; j++)
    {

        elf_program_header *header = elf->program_headers[j];
        /* load */
        if (header->type == 1)
        {

            for (size_t i = 0; i <= header->size_memory + PAGE_SIZE;
                 i += PAGE_SIZE)
            {

                addr_space_map(&t->addr_space,
                               (void *)(header->virtual_addr + i), 1,
                               PAGING_PRESENT | PAGING_USER | PAGING_PRESENT);
                /* map_page((void *)get_free_phys_page(), */
                /*          (void *)(header->virtual_addr + i), */
                /*          PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT); */
            }

            /* this may not have zeros at the end */
            u8 *buf = malloc(header->size_memory);

            pread(elf->fd, buf, header->size_file, header->file_offset);

            if (header->size_memory > header->size_file)
            {

                for (size_t i = 0; i < header->size_memory - header->size_file;
                     i++)
                {
                    buf[header->size_file + i] = 0;
                }
            }

            uint8_t *dest = (uint8_t *)(header->virtual_addr);

            for (size_t i = 0; i < header->size_memory; i++)
            {

                dest[i] = buf[i];
            }
        }
        else
        {
            panic("unrecognized header type:%d when loading elf file\n",
                  header->type);
        }
    }

    create_userspace_stack(pgdir);

    /* *(char ***)(0xBFFEF000 + sizeof(ptr)) = args; */
    /* *(int *)(0xBFFEF000) = argc; */

    load_pgdir(temp_pgdir);

    return t;
}
