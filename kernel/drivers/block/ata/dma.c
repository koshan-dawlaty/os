#include "ata_utils.h"
#include "kernel/paging.h"
#include "utils.h"
#include <ata/ata.h>
#include <compiler.h>
#include <memory/get_free_pages.h>
#include <stdint.h>

struct PRD_entry
{
    u32 buffer_addr;
    u16 transfer_size;
    u16 reserved; /* set MSB in this for last entry */
} __packed;

#define SECTORS_PER_BUFFER 2

u16 ata_bus_master_register = 0;

/*
 * creates a new prd entry with a buffer holding SECTORS_PER_BUFFER sectors
 * modifies virtual_addr to be the virtual address of the buffer
 */
struct PRD_entry get_prd_entry(void **virutal_addr)
{
    size_t amount = (((SECTORS_PER_BUFFER * 512) + PAGE_SIZE) / PAGE_SIZE);
    logd("num pages:%d", amount);

    void *pages = dma_alloc_pages(amount);
    void *phys = get_physaddr(pages);
    logd("prd ent phys:%p", phys);

    logd("vaddr:%p", pages);

    ASSERT((ptr_t)phys < ((ptr_t)1 << 32),
           "address %p needs within 32 bit address space", phys);
    *virutal_addr = pages;

    return (struct PRD_entry){(u32)(ptr_t)phys, SECTORS_PER_BUFFER * 512, 0};
}

void read_ata_dma(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    logd("sectors:%d", sectors);

    ata_isr_handled = 0;
    /* TODO: change size */
    struct PRD_entry *prdt = dma_alloc_pages(1);
    struct PRD_entry *prdt_phys = get_physaddr(prdt);

    logd("prdt_phys:%p", prdt_phys);

    ptr_t virtual_addrs[sectors / SECTORS_PER_BUFFER];
    size_t virtual_addrs_index = 0;

    size_t prdt_index = 0;

    for (size_t i = 0; i < sectors; i += SECTORS_PER_BUFFER)
    {
        void *virtual_addr;
        struct PRD_entry entry = get_prd_entry(&virtual_addr);

        virtual_addrs[virtual_addrs_index] = (ptr_t)virtual_addr;
        virtual_addrs_index++;

        prdt[prdt_index] = entry;
        prdt_index++;
    }

    prdt[prdt_index - 1].reserved = 0x8000;

    outb(ata_bus_master_register, 0);
    outb(ata_bus_master_register + 2, 0x06);
    outl(ata_bus_master_register + 4, (u32)(ptr_t)prdt_phys);
    outb(ata_bus_master_register, 0);

    /* ata_drive_select(io_offset, control_offset, 0xE0 | ((lba >> 24) & 0x0F));
     */

    ata_send_command_24(io_offset, control_offset, sectors, lba, 0xC8);

    outb(ata_bus_master_register, 1);

    /* printf("%p\n", prdt_phys); */
    /* printf("%p\n", prdt[0]); */
    logd("ata bmr %p", ata_bus_master_register);

    /* outl(ata_bus_master_register + 4, (u32)(ptr_t)prdt_phys); */
    /* outb(ata_bus_master_register, 1); */
    /* outb(ata_bus_master_register, 1 | 8); */

    /* outb(ata_bus_master_register + 2, 4); */
    /* logd("status:%p", inb(ata_bus_master_register + 2)); */

    /* ata_drive_select(io_offset, control_offset, */
    /*                  0xE0 |  ((lba >> 24) & 0x0F)); */
    /* ata_send_command_24(io_offset, control_offset, sectors, lba, 0xC8); */

    /* outw(ata_bus_master_register + 4, (uint32_t)prdt_phys & 0xFF); */
    /* outw(ata_bus_master_register + 6, ((uint32_t)(prdt_phys) >> 8) & 0xFF);
     */

    /* printf("%p\n", inb(ata_bus_master_register + 2)); */

    while (!ata_isr_handled)
    {
        irq_wait();
    }
    ata_isr_handled = 0;

    wait_for_bit(io_offset, control_offset, 7, 0);

    u8 ata_status = inb(io_offset + 7);
    u8 ata_error = inb(io_offset + 1);
    logd("ata: %d, %d", ata_status, ata_error);

    logd("status:%p", inb(ata_bus_master_register + 2));
    u8 command = inb(ata_bus_master_register);
    logd("command:%p", command);
    outb(ata_bus_master_register, command & ~1);
    logd("command:%p", command);

    logd("status:%p", inb(ata_bus_master_register + 2));

    /* wait_ata1_isr(); */

    /* printf("%p\n", ((uint16_t *)(virtual_addrs[0]))[0]); */

    for (size_t sector = 0; sector < sectors; sector++)
    {
        logd("reading at:%p", virtual_addrs[sector / SECTORS_PER_BUFFER]);
        logd("%p",
             ((u8 *)((ptr_t)virtual_addrs[sector / SECTORS_PER_BUFFER]))[0]);
        for (int i = 0; i < 512; i++)
        {
            u8 value =
                ((u8 *)((ptr_t)virtual_addrs[sector / SECTORS_PER_BUFFER]))[i];
			if (value != 0) {
				logd("got %p", value);
			}
            /* logd("%p", ((u8 *)((ptr_t)virtual_addrs[sector /
             * SECTORS_PER_BUFFER]))[i]); */
            /* printf( */
            /*     "%p", */
            /*     ((u16 *)((ptr_t)virtual_addrs[sector /
             * SECTORS_PER_BUFFER]))[i]); */
        }
    }
}
