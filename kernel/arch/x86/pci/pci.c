#include "pci/pci.h"
#include "ata/ata.h"
#include "drivers/character/e1000.h"
#include "lib/log.h"
#include "utils.h"
#include <stdint.h>

u32 pci_config_get_addr(u8 bus, u8 dev, u8 func, u8 offset)
{
    u32 lbus = (u32)bus;
    u32 ldev = (u32)dev;
    u32 lfunc = (u32)func;

    u32 addr = (u32)((lbus << 16) | (ldev << 11) | (lfunc << 8) |
                     /* remove lowest 2 bits from offset */ (offset & 0xFC) |
                     ((u32)0x80000000));
    return addr;
}

uint16_t read_pci_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{

    outl(0xCF8, pci_config_get_addr(bus, dev, func, offset));

    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    return (u16)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}
void pci_write_config(u8 bus, u8 dev, u8 func, u8 offset, u32 value)
{
    outl(0xCF8, pci_config_get_addr(bus, dev, func, offset));
    outl(0xCFC, value);
}

u32 pci_read_config_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    u16 one = read_pci_config(bus, dev, func, offset);
    u16 two = read_pci_config(bus, dev, func, offset + 2);
    return (u32)one | ((u32)two << 16);
}

// TODO: maybe improve later and also move to other file

/*
 * initialize device for other subsystems
 */
void parse_device(uint8_t bus, uint8_t dev, uint8_t func)
{
    uint16_t class_word = read_pci_config(bus, dev, func, 0xA);

    uint8_t base = (uint8_t)((class_word >> 8) & 0xFF);
    uint8_t sub = (uint8_t)(class_word & 0xFF);

    u16 vendor = read_pci_config(bus, dev, func, 0);
    u16 device = read_pci_config(bus, dev, func, 2);

    if (vendor == 0x8086 && device == 0x100E)
    {
        /* u32 bar0 = pci_read_config_word(bus, dev, func, 0x10); */
        /* u32 word = pci_read_config_word(bus, dev, func, 4); */
        /* word |= (1 << 2); */
        /* pci_write_config(bus, dev, func, 4, word); */
        /* e1000_send(e, "abc", 4); */
    }

    /* logd("device!!"); */

    if (base == 1 && sub == 1)
    {
        logd("ide controller found");

        u16 command = pci_read_config_word(bus, dev, func, 4);

        command |= 4;

        pci_write_config(bus, dev, func, 4, command);
        u16 check = pci_read_config_word(bus, dev, func, 4);

        ASSERT(check & 4, "pci busmastering failed to enable");

        u32 bar4 = pci_read_config_word(bus, dev, func, 0x20);
        if (bar4 & 1)
        {
            ata_bus_master_register = bar4 & 0xFFFC;
        }
        else
        {
            PANIC("memory ata bus master register found!");
        }
        /* logd("ata register:%p", ata_bus_master_register); */
    }
}

void check_device(uint8_t bus, uint8_t dev);

void check_bus(uint8_t bus)
{

    for (int i = 0; i < 32; i++)
    {
        check_device(bus, i);
    }
}

/*
 * checks for a pci bridge
 */
void check_function(uint8_t bus, uint8_t dev, uint8_t func)
{

    uint16_t class_word = read_pci_config(bus, dev, func, 0xA);

    uint8_t base = (uint8_t)((class_word >> 8) & 0xFF);
    uint8_t sub = (uint8_t)(class_word & 0xFF);

    if (base == 6 && sub == 4)
    {
        logd("valid funciton");

        u8 secondary_bus = (read_pci_config(bus, dev, func, 0x18) >> 8) & 0xFF;
        check_bus(secondary_bus);
    }
}

void check_device(uint8_t bus, uint8_t dev)
{
    uint8_t func = 0;

    uint16_t vendor_id = read_pci_config(bus, dev, func, 0);

    if (vendor_id == 0xFFFF)
    {
        return;
    }

    uint16_t type = read_pci_config(bus, dev, func, 0xE);

    check_function(bus, dev, func);
    parse_device(bus, dev, func);

    if (type & 0x80)
    {
        for (func = 1; func < 8; func++)
        {
            if (read_pci_config(bus, dev, func, 0x0) != 0xFFFF)
            {
                check_function(bus, dev, func);
                parse_device(bus, dev, func);
            }
        }
    }
}

void check_all_buses()
{

    u8 type = read_pci_config(0, 0, 0, 0xE);

    if ((type & 0x80))
    {
        // multiple host controllers
        for (uint8_t func = 0; func < 8; func++)
        {

            if (read_pci_config(0, 0, func, 0) != 0xFFFF)
            {
                check_bus(func);
            }
        }
    }
    else
    {
        check_bus(0);
    }
}

void init_pci()
{
    check_all_buses();
}
