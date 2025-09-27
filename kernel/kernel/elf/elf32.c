#include "elf.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "unistd.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    u32 type;
    u32 file_offset;
    u32 virutal_addr;
    u32 physical_addr;
    u32 file_size;
    u32 memory_size;
    u32 flags;
    u32 align;
} elf_program_header_32;

elf *parse_elf32(int fd)
{
    uint8_t *header_buf = malloc(52);
    pread(fd, header_buf, 52, 0);
    if (header_buf[0] != 0x7F || header_buf[1] != 'E' || header_buf[2] != 'L' ||
        header_buf[3] != 'F')
    {
        printf("invalid elf header\n");
        return NULL;
    }
    elf *out = malloc(sizeof(elf));
    uint32_t program_header_table_offset = word_from_buffer(header_buf + 28);
    uint32_t program_header_size = short_from_buffer(header_buf + 42);

    out->entry_point = word_from_buffer(header_buf + 32);

    out->program_headers_len = short_from_buffer(header_buf + 44);

    uint8_t *program_headers_buf =
        malloc(program_header_size * out->program_headers_len);

    pread(fd, program_headers_buf,
          program_header_size * out->program_headers_len,
          program_header_table_offset);

    elf_program_header **program_headers =
        malloc(sizeof(elf_program_header_32 *) * out->program_headers_len);
    for (size_t i = 0; i < out->program_headers_len; i++)
    {
        elf_program_header_32 *raw_header =
            (elf_program_header_32 *)(program_headers_buf) +
            (i * program_header_size);

        elf_program_header *new_header = malloc(sizeof(elf_program_header));

        new_header->align = raw_header->align;
        new_header->file_offset = raw_header->file_offset;
        new_header->size_file = raw_header->file_size;
        new_header->size_memory = raw_header->memory_size;
        new_header->flags = raw_header->flags;
        new_header->type = raw_header->type;
        new_header->virtual_addr = raw_header->virutal_addr;
        new_header->physical_addr = raw_header->physical_addr;

        program_headers[i] = new_header;
    }
    out->program_headers = program_headers;

    free(program_headers_buf);

    out->fd = fd;

    free(header_buf);

    return out;
}
