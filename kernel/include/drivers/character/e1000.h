#ifndef E1000_NETWORK_H
#define E1000_NETWORK_H

#include <types.h>

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8
struct e1000
{
    bool eeprom_exists;
    u8 mac[6];
    struct e1000_rx_desc *rx_descs[E1000_NUM_RX_DESC];
    struct e1000_tx_desc *tx_descs[E1000_NUM_TX_DESC];
    u16 current_rx;
    u16 current_tx;

    u8 bar_type;
    u16 io_base;
    ptr_t mem_base;
};
struct e1000 *e1000_alloc(u32 bar0);
bool e1000_init(struct e1000 *dev);
int e1000_send(struct e1000 *dev, const void *data, u16 len);

#endif
