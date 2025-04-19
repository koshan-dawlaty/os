#include "kernel/paging.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"
#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void jump_usermode_asm(uint32_t addr);

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

    out->program_headers_len = short_from_buffer(header_buf + 44);
    out->entry_point = word_from_buffer(header_buf + 24);

    uint8_t *program_headers_buf =
        malloc(program_header_size * out->program_headers_len);
    pread(fd, program_headers_buf,
          program_header_size * out->program_headers_len,
          program_header_table_offset);
    /* printf("offset:%d\n", program_header_table_offset); */

    elf_program_header_32 **program_headers =
        malloc(sizeof(elf_program_header_32 *));

    for (size_t i = 0; i < out->program_headers_len; i++)
    {
        elf_program_header_32 *new_header =
            malloc(sizeof(elf_program_header_32));

        memcpy(new_header, (program_headers_buf) + (i * program_header_size),
               sizeof(elf_program_header_32));

        /* printf("%d\n", new_header->file_offset); */
        program_headers[i] = new_header;
    }

    out->program_headers = program_headers;

    return out;
}

/* TODO: MAKE THIS USE GRUB MEMORY MAP */
uint32_t elf_physaddr_start = 0x2000000;

void load_elf32(elf_32 *elf)
{

    for (size_t j = 0; j < elf->program_headers_len; j++)
    {
        elf_program_header_32 *header = elf->program_headers[j];

        /* printf("memory:%d, file:%d, offset:%d\n", header->memory_size, */
        /*        header->file_size, header->file_offset); */

        // load
        if (header->type == 1)
        {

            printf("memory_size:%p file_size:%p virutal addr:%x phys addr:%x\n",
                   header->memory_size, header->file_size, header->virutal_addr,
                   header->physical_addr);

            for (size_t i = 0; i <= header->memory_size + PAGE_SIZE;
                 i += PAGE_SIZE)
            {
                printf("i:%x, mapping: %p\n", i,
                       align_long(header->virutal_addr + (i), PAGE_SIZE));

                /* map_page((void *)(header->physical_addr + (i)), */
                /*          (void *)(header->virutal_addr + (i)), */
                /*          PAGING_WRITEABLE | PAGING_USER); */
                map_page((void *)(elf_physaddr_start),
                         (void *)(header->virutal_addr + (i)),
                         PAGING_WRITEABLE | PAGING_USER);

                elf_physaddr_start += 4096;
                /* map_page((void *)(0xB424000 + (i)), */
                /*          (void *)(header->virutal_addr + (i)), 7); */
            }

            /* this may not have zeros at the end */
            uint8_t *buf = malloc(header->memory_size);

            /* if (header->memory_size > header->file_size) */
            /* { */
            /*     for (size_t i = 0; i < header->memory_size -
             * header->file_size; */
            /*          i++) */
            /*     { */
            /*         buf[header->file_size + i] = 0; */
            /*     } */
            /* } */

            pread(elf->fd, buf, header->file_size, header->file_offset);

            for (size_t i = 0; i < 10; i++)
            {
                /* printf("%p ", buf[i]); */
            }

            /* printf("virutal_addr: %p\n", header->virutal_addr); */

            uint8_t *dest = (uint8_t *)(header->virutal_addr);

            /* printf("size:%p\n", header->file_size); */
            /* for (size_t i = 0; i < 10; i++) */
            /* { */
            /*     dest[i] = buf[i]; */
            /* } */

            /* printf("%d\n", *((uint8_t *)(header->virutal_addr))); */
            /* printf("%d\n", *((uint8_t *)(header->virutal_addr))); */

            /* *(uint8_t *)(header->virutal_addr) = 'a'; */
            /* *(char *)(0xC03FF004) = *(uint8_t *)(header->virutal_addr); */
            /* *(char *)(0xC03FF002) = *(uint8_t *)(header->virutal_addr); */

            /* dest[0] = 'a'; */
            /* *(char *)(0xC03FF000) = dest[0]; */
            /* *(char *)(0xC03FF002) = dest[0]; */

            /* *(char *)(0xC03FF004) = dest[1]; */
            /* *(char *)(0xC03FF006) = dest[1]; */
            memcpy(dest, buf, header->file_size);

            for (size_t i = 0; i < header->file_size; i++)
            {
                /* dest[i] = buf[i]; */
                /* printf("i: %d, dest:%p src:%p\n", i, dest[i], buf[i]); */
                /* printf("i: %d, dest:%p src:%p\n", i, dest[i], buf[i]); */

                if (buf[i] != dest[i])
                {
                    printf("%p != %p\n", buf[i], dest[i]);
                }
            }

            /* printf("test\n"); */

            free(buf);
        }
    }

    printf("%p\n", elf->entry_point);

    jump_usermode_asm(elf->entry_point);
    /* ((void (*)())(elf->entry_point))(); */

    /* printf("done\n"); */

    /* printf("%p\n", *(uint8_t *)(elf->entry_point)); */
    /* printf("%p\n", *(uint8_t *)(elf->entry_point + 1)); */
    /* for (int i = 0; i < 100; i++) */
    /* { */
    /*     printf("%p ", */
    /* ((uint8_t *)(elf->program_headers[0]->virutal_addr))[i]); */
    /* } */

    /* printf("entry:%p\n", elf->entry_point); */

    /* printf("done\n"); */

    /* asm volatile("jmp *%0" : : "r"(elf->entry_point) :); */
}
