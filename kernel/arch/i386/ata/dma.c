#include "ata/ata.h"
#include "ata/ata_utils.h"
#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct PRD_entry
{
    uint32_t buffer_addr;
    uint16_t transfer_size;
    uint16_t reserved; // set MSB in this for last entry
} __attribute__((packed));

#define SECTORS_PER_BUFFER 8
// TODO: this will cause conflicts (probably with get_free_pages)
#define VIRTUAL_ADDR_START 0xD9000000
uint32_t virtual_addr_offset = 0;

uint16_t ata_bus_master_register = 0;

/*
 * creates a new prd entry with a buffer holding SECTORS_PER_BUFFER sectors
 * modifies virtual_addr to be the virtual address of the buffer
 */
struct PRD_entry get_prd_entry(uint32_t *virutal_addr)
{
    size_t amount = ((SECTORS_PER_BUFFER * 512) / PAGE_SIZE) + 1;
    void *buf = get_free_phys_pages(amount);

    for (size_t i = 0; i < amount; i++)
    {
        map_page(buf, (void *)(VIRTUAL_ADDR_START + virtual_addr_offset),
                 PAGING_WRITEABLE);
        *virutal_addr = VIRTUAL_ADDR_START + virtual_addr_offset;
        virtual_addr_offset += PAGE_SIZE;
    }

    return (struct PRD_entry){(uint32_t)buf, amount * PAGE_SIZE, 0x8000};
}

void read_ata_dma(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    printf("%p\n", inb(ata_bus_master_register + 2));
    // TODO: change size
    struct PRD_entry *prdt_phys = get_free_phys_pages(1);

    map_page(prdt_phys, (void *)(VIRTUAL_ADDR_START + virtual_addr_offset),
             PAGING_WRITEABLE);

    struct PRD_entry *prdt = (void *)VIRTUAL_ADDR_START + virtual_addr_offset;

    uint32_t virtual_addrs[sectors / SECTORS_PER_BUFFER] = {};
    size_t virtual_addrs_index = 0;

    virtual_addr_offset += PAGE_SIZE;

    size_t prdt_index = 0;

    for (size_t i = 0; i < sectors; i += SECTORS_PER_BUFFER)
    {

        uint32_t addr = 0;
        struct PRD_entry entry = get_prd_entry(&addr);

        virtual_addrs[virtual_addrs_index] = addr;
        virtual_addrs_index++;

        prdt[prdt_index] = entry;
        prdt_index++;
    }

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));
    ata_send_command_24(io_offset, control_offset, sectors, lba, 0xC8);
    outw(ata_bus_master_register + 4, (uint32_t)prdt_phys & 0xFF);
    outw(ata_bus_master_register + 6, ((uint32_t)(prdt_phys) >> 8) & 0xFF);
    outw(ata_bus_master_register, 1 | (1 << 2));
    printf("%p\n", inb(ata_bus_master_register + 2));
    wait_ata1_isr();

    /* printf("%p\n", ((uint16_t *)(virtual_addrs[0]))[0]); */

    for (size_t sector = 0; sector < sectors; sector++)
    {
        for (int i = 0; i < 512; i++)
        {
            printf(
                "%p",
                ((uint16_t *)(virtual_addrs[sector / SECTORS_PER_BUFFER]))[i]);
        }
    }
}
