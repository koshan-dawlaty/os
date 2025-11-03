#ifndef ATA_H
#define ATA_H

// #include "interrupts/idt.h"
#include "drivers/block/bdev.h"
#include <kernel/types.h>
#include <stddef.h>
#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

// kinda stupid
#define ATA_PRIMARY ATA_PRIMARY_IO, ATA_PRIMARY_CONTROL

#define ATA_DATA 0
#define ATA_ERROR 1
#define ATA_FEATURES 1
#define ATA_SECTOR_COUNT 2
#define ATA_LBA_LO 3
#define ATA_LBA_MID 4
#define ATA_LBA_HI 5
#define ATA_DRIVE_HEAD 6
#define ATA_STATUS 7
#define ATA_COMMAND 7

// TODO: maybe have a way of differentiating io and control registers
#define ATA_ALT_STATUS 0
#define ATA_CONTROL 0
#define ATA_DRIVE_ADDRESS 1

#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CONTROL 0x376

#define ATA_SECONDARY ATA_SECONDARY_IO, ATA_SECONDARY_CONTROL

void init_ata();

// ISR_ATTRIBUTE void ata_isr_1(struct interrupt_frame *);
// ISR_ATTRIBUTE void ata_isr_2(struct interrupt_frame *);

// TODO: move to other header file

/*
 * buffer should be 512 * sectors bytes long
 */
void read_ata_pio(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba);
/*
 * buffer_size is in bytes
 */
void write_ata_pio(uint16_t io_offset, uint16_t control_offset, uint32_t lba,
                   uint8_t *buffer, size_t buffer_size);

extern uint16_t ata_bus_master_register;

typedef struct
{
    struct
    {
        uint8_t is_dma : 1;
        uint8_t is_primary : 1;
        uint8_t is_drive1 : 1;
        uint8_t is_lba48 : 1;
    };

    uint16_t io_offset;
    uint16_t control_offset;

} ata_drive;

/*
 * drive num is 0-3
 *
 *
 * drive num of 0 is primary bus drive 1
 *
 * drive num of 1 is primary bus drive 2
 *
 * drive num of 2 is secondary bus drive 1
 *
 * drive num of 3 is secondary bus drive 2
 *
 * can return NULL
 */
struct bdev *get_ata_drive(u8 drive_num);

extern struct bdev_functions ata_bdev_functions;

#endif
