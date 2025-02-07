#include "ata/ata.h"
#include "ata/ata_utils.h"
#include <stdint.h>

struct __attribute__((packed))
{
    uint32_t buffer_addr;
    uint16_t transfer_size;
    uint16_t reserved; // set MSB in this for last entry
} PRD_entry;

#define SECTORS_PER_BUFFER 2

uint16_t ata_bus_master_register = 0;

void read_ata_dma(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    ata_send_command_24(io_offset, control_offset, sectors, lba, 0xC8);
}
