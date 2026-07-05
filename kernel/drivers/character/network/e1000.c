#include "e1000.h"
#include "kernel/paging.h"
#include "lib/log.h"
#include "memory/paging_flags.h"
#include <drivers/character/e1000.h>
#include <memory/get_free_pages.h>
#include <stddef.h>
#include <stdlib.h>
#include <utils.h>

void e1000_write_command(struct e1000 *dev, u16 address, u32 value)
{
    if (dev->bar_type == 0)
    {
        *((volatile u32 *)(dev->mem_base + address)) = value;
    }
    else
    {
        outl(dev->io_base, address);
        outl(dev->io_base + 4, value);
    }
}

u32 e1000_read_command(struct e1000 *dev, u16 address)
{
    if (dev->bar_type == 0)
    {
        return *((volatile u32 *)(dev->mem_base + address));
    }
    else
    {
        outl(dev->io_base, address);
        return inl(dev->io_base + 4);
    }
}

bool e1000_detect_eeprom(struct e1000 *dev)
{
    e1000_write_command(dev, REG_EEPROM, 0x1);
    bool eeprom_exists = false;

    for (size_t i = 0; i < 1000; i++)
    {
        u32 val = e1000_read_command(dev, REG_EEPROM);
        if (val & 0x10)
        {
            eeprom_exists = true;
            break;
        }
    }
    dev->eeprom_exists = eeprom_exists;
    return eeprom_exists;
}

u32 e1000_eeprom_read(struct e1000 *dev, u8 addr)
{
    u16 data = 0;
    u32 tmp = 0;
    if (dev->eeprom_exists)
    {
        e1000_write_command(dev, REG_EEPROM, (1) | ((u32)(addr) << 8));
        while (!((tmp = e1000_read_command(dev, REG_EEPROM)) & (1 << 4)))
        {
        }
    }
    else
    {
        e1000_write_command(dev, REG_EEPROM, (1) | ((u32)(addr) << 2));
        while (!((tmp = e1000_read_command(dev, REG_EEPROM)) & (1 << 1)))
        {
        }
    }
    data = (u16)((tmp >> 16) & 0xFFFF);
    return data;
}
bool e1000_read_mac_address(struct e1000 *dev)
{
    if (dev->eeprom_exists)
    {
        uint32_t temp;
        temp = e1000_eeprom_read(dev, 0);
        dev->mac[0] = temp & 0xff;
        dev->mac[1] = temp >> 8;
        temp = e1000_eeprom_read(dev, 1);
        dev->mac[2] = temp & 0xff;
        dev->mac[3] = temp >> 8;
        temp = e1000_eeprom_read(dev, 2);
        dev->mac[4] = temp & 0xff;
        dev->mac[5] = temp >> 8;
    }
    else
    {
        u8 *mem_base_mac_8 = (u8 *)(dev->mem_base + 0x5400);
        u32 *mem_base_mac_32 = (u32 *)(dev->mem_base + 0x5400);
        if (mem_base_mac_32[0] != 0)
        {
            for (int i = 0; i < 6; i++)
            {
                dev->mac[i] = mem_base_mac_8[i];
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

void *dma_malloc(size_t size)
{
    /* TODO: this should be a kmalloc zone */
    size_t num_pages = (size / PAGE_SIZE) + 1;
    void *physaddr = get_free_phys_pages(get_current_pagedir(), num_pages);
    return map_physaddr(physaddr, num_pages,
                        PAGING_PRESENT | PAGING_WRITEABLE |
                            PAGING_WRITE_THROUGH_CACHE);
}

void e1000_rxinit(struct e1000 *dev)
{
    u8 *ptr;
    struct e1000_rx_desc *descs;

    ptr = (u8 *)(dma_malloc(sizeof(struct e1000_rx_desc) * E1000_NUM_RX_DESC +
                            16));

    descs = (struct e1000_rx_desc *)ptr;
    for (int i = 0; i < E1000_NUM_RX_DESC; i++)
    {
        dev->rx_descs[i] = (struct e1000_rx_desc *)((uint8_t *)descs + i * 16);
        dev->rx_descs[i]->addr = (uint64_t)(uint8_t *)(dma_malloc(8192 + 16));
        dev->rx_descs[i]->status = 0;
    }

    e1000_write_command(dev, REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(dev, REG_TXDESCHI,
                        (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    e1000_write_command(dev, REG_RXDESCLO, (uint64_t)ptr);
    e1000_write_command(dev, REG_RXDESCHI, 0);

    e1000_write_command(dev, REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    e1000_write_command(dev, REG_RXDESCHEAD, 0);
    e1000_write_command(dev, REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
    dev->current_rx = 0;
    e1000_write_command(dev, REG_RCTRL,
                        RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE |
                            RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM |
                            RCTL_SECRC | RCTL_BSIZE_8192);
}
void e1000_txinit(struct e1000 *dev)
{
    u8 *ptr;
    struct e1000_tx_desc *descs;
    ptr = (uint8_t *)(dma_malloc(
        sizeof(struct e1000_tx_desc) * E1000_NUM_TX_DESC + 16));

    descs = (struct e1000_tx_desc *)ptr;
    for (int i = 0; i < E1000_NUM_TX_DESC; i++)
    {
        dev->tx_descs[i] = (struct e1000_tx_desc *)((uint8_t *)descs + i * 16);
        dev->tx_descs[i]->addr = 0;
        dev->tx_descs[i]->cmd = 0;
        /* dev->tx_descs[i]->status = TSTA_DD; */
        dev->tx_descs[i]->status = 0;
    }
    asm volatile("" ::: "memory");

    e1000_write_command(dev, REG_TXDESCHI, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(dev, REG_TXDESCLO,
                        (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    // now setup total length of descriptors
    e1000_write_command(dev, REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);

    // setup numbers
    e1000_write_command(dev, REG_TXDESCHEAD, 0);
    e1000_write_command(dev, REG_TXDESCTAIL, 0);
    dev->current_tx = 0;
    e1000_write_command(dev, REG_TCTRL,
                        TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) |
                            (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);

    /* this was from the osdev wiki article for this ethernet card, it seems
     * like this only applies on real hardware? */
    // This line of code overrides the one before it but I left both to
    // highlight that the previous one works with e1000 cards, but for the
    // e1000e cards you should set the TCTRL register as follows. For detailed
    // description of each bit, please refer to the Intel Manual. In the case of
    // I217 and 82577LM packets will not be sent if the TCTRL is not configured
    // using the following bits.
    /* e1000_write_command(dev, REG_TCTRL, 0b0110000000000111111000011111010);
     */

    e1000_write_command(dev, REG_TIPG, 0x0060200A);
}
void e1000_interrupts_enable(struct e1000 *dev)
{
    e1000_write_command(dev, REG_IMASK, 0x1F6DC);
    e1000_write_command(dev, REG_IMASK, 0xff & ~4);
    e1000_read_command(dev, 0xc0);
}

void e1000_handle_receive(struct e1000 *dev)
{
    u16 old_current;
    bool got_packet = false;

    while ((dev->rx_descs[dev->current_rx]->status & 0x1))
    {
        got_packet = true;
        u8 *buf = (u8 *)dev->rx_descs[dev->current_rx]->addr;
        u16 len = dev->rx_descs[dev->current_rx]->length;

        // Here you should inject the received packet into your network stack

        dev->rx_descs[dev->current_rx]->status = 0;
        old_current = dev->current_rx;
        dev->current_rx = (dev->current_rx + 1) % E1000_NUM_RX_DESC;
        e1000_write_command(dev, REG_RXDESCTAIL, old_current);
    }
}
int e1000_send(struct e1000 *dev, const void *data, u16 len)
{
    logd("%p", dev->tx_descs[0]);
    logd("current_tx:%d", dev->current_tx);
    dev->tx_descs[dev->current_tx]->addr = (ptr_t)data;
    dev->tx_descs[dev->current_tx]->length = len;
    dev->tx_descs[dev->current_tx]->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    dev->tx_descs[dev->current_tx]->status = 0;
    u8 old_cur = dev->current_tx;
    dev->current_tx = (dev->current_tx + 1) % E1000_NUM_TX_DESC;
    e1000_write_command(dev, REG_TXDESCTAIL, dev->current_tx);
    while (!(dev->tx_descs[old_cur]->status & 0xff))
    {
    }
    logd("done sending");

    return 0;
}

bool e1000_init(struct e1000 *dev)
{
    e1000_detect_eeprom(dev);
    if (!e1000_read_mac_address(dev))
    {
        return false;
    }
    log_multiline_start(&LOG_INFO);
    logi("mac address: ");
    for (size_t i = 0; i < 6; i++)
    {
        logi("%p ", dev->mac[i]);
    }
    log_multiline_end(&LOG_INFO);

    /* for (int i = 0; i < 0x80; i++) */
    /* { */
    /*     e1000_write_command(dev, 0x5200 + i * 4, 0); */
    /* } */

    /* TODO: register interrupt handler */
    e1000_txinit(dev);
    e1000_rxinit(dev);

    return true;
}

struct e1000 *e1000_alloc(u32 bar0)
{
    struct e1000 *out = malloc(sizeof(struct e1000));
    out->bar_type = bar0 & 1;
    if (out->bar_type == 0)
    {

        /* FIXME: this probably doesnt work for a 64 bit physical address */
        void *paddr = (void *)((ptr_t)bar0 & 0xFFFFFFF0);
        logd("paddr:%p", paddr);
        out->mem_base = (ptr_t)map_physaddr(paddr, 10,
                                            PAGING_WRITEABLE | PAGING_PRESENT |
                                                PAGING_WRITE_THROUGH |
                                                PAGING_WRITE_THROUGH_CACHE);
    }
    else
    {
        out->io_base = (bar0 & 0xFFFFFFFC);
    }
    /* TODO: enable bus mastering */
    out->eeprom_exists = false;

    return out;
}
