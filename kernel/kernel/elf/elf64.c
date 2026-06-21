#include <elf.h>
#include <kernel/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <unistd.h>

typedef struct
{
    int fd;
    u16 program_headers_len;
    elf_program_header **program_headers;
} elf_64;

typedef struct
{
    u32 type;
    u32 flags;
    ptr_t offset;
    ptr_t virtual_addr;
    ptr_t physical_addr;
    u64 file_size;
    u64 memory_size;
    u64 align;

} elf_program_header_64;

elf *parse_elf64(int fd)
{
    uint8_t *header_buf = malloc(64);
    pread(fd, header_buf, 64, 0);
    if (header_buf[0] != 0x7F || header_buf[1] != 'E' || header_buf[2] != 'L' ||
        header_buf[3] != 'F')
    {
        return NULL;
    }
    elf *out = malloc(sizeof(elf));
    u64 program_header_table_offset = long_from_buffer(header_buf + 32);
    u16 program_header_size = short_from_buffer(header_buf + 54);

    out->entry_point = long_from_buffer(header_buf + 24);

    out->program_headers_len = short_from_buffer(header_buf + 56);
    out->program_headers =
        malloc(out->program_headers_len * sizeof(elf_program_header));

    free(header_buf);

    u8 *program_header_buf =
        malloc(program_header_size * out->program_headers_len);
    pread(fd, program_header_buf,
          program_header_size * out->program_headers_len,
          program_header_table_offset);

    for (size_t i = 0; i < out->program_headers_len; i++)
    {

        elf_program_header_64 *raw_header =
            (elf_program_header_64 *)(program_header_buf +
                                      (i * program_header_size));

        elf_program_header *new_header = malloc(sizeof(elf_program_header));
        new_header->type = raw_header->type;
        new_header->flags = raw_header->flags;
        new_header->file_offset = raw_header->offset;
        new_header->virtual_addr = raw_header->virtual_addr;
        new_header->physical_addr = raw_header->physical_addr;
        new_header->size_file = raw_header->file_size;
        new_header->size_memory = raw_header->memory_size;
        new_header->align = raw_header->align;

        out->program_headers[i] = new_header;
    }

    free(program_header_buf);

    out->fd = fd;

    return out;
}
