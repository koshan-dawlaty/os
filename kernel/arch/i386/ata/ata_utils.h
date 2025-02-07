#ifndef ATA_UTILS_H
#define ATA_UTILS_H

#include <stdint.h>

/*
 * waits for bit to be set if set is true, otherwise waits for it to clear
 *
 * bit is from 0-7
 */
void wait_for_bit(uint16_t io_offset, uint16_t control_offset, uint8_t bit,
                  uint8_t set);

/*
 * reads status register 15 times to correctly select a drive
 *
 * very slow
 */
void ata_drive_select(uint16_t io_offset, uint16_t control_offset,
                      uint8_t drive);

void ata_send_command_24(uint16_t io_offset, uint16_t control_offset,
                         uint8_t sectors, uint32_t lba, uint8_t command);

void ata_send_command_48(uint16_t io_offset, uint16_t control_offset,
                         uint8_t sectors, uint64_t lba, uint8_t command);
#endif
