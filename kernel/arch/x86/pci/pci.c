#include "pci/pci.h"
#include "ata/ata.h"
#include "utils.h"
#include <stdint.h>

uint16_t read_pci_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{

    uint32_t lbus = (uint32_t)bus;
    uint32_t ldev = (uint32_t)dev;
    uint32_t lfunc = (uint32_t)func;

    uint32_t addr =
        (uint32_t)((lbus << 16) | (ldev << 11) | (lfunc << 8) |
                   /* remove lowest 2 bits from offset */ (offset & 0xFC) |
                   ((uint32_t)0x80000000));

    outl(0xCF8, addr);

    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    return (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
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

    if (base == 1 && sub == 1)
    {
        // ide controller

        ata_bus_master_register = read_pci_config(bus, dev, func, 0x20);
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

        uint8_t secondary_bus =
            (read_pci_config(bus, dev, func, 0x18) >> 8) & 0xFF;
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

    uint8_t type = read_pci_config(bus, dev, func, 0xE);

    check_function(bus, dev, func);

    if (type & 0x80)
    {
        for (func = 1; func < 8; func++)
        {
            if (read_pci_config(bus, dev, func, 0x0) != 0xFFFF)
            {
                check_function(bus, dev, func);
            }
        }
    }
}

void check_all_buses()
{

    uint8_t type = read_pci_config(0, 0, 0, 0xE);

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
