#include "ata.h"
#include "interrupts/pic.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

volatile uint8_t ata1_isr_sent = 0;
volatile uint8_t ata2_isr_sent = 0;

void wait_ata1_isr()
{
    while (!ata1_isr_sent)
    {
        asm volatile("hlt");
    }
    ata1_isr_sent = 0;
}

void wait_ata2_isr()
{
    while (!ata2_isr_sent)
    {
        asm volatile("hlt");
    }
    ata2_isr_sent = 0;
}

ISR(ata_isr_1, {
    printf("ata isr 1\n");

    ata1_isr_sent = 1;

    PIC_sendEOI(14);
})

ISR(ata_isr_2, {
    printf("ata isr 2\n");

    ata2_isr_sent = 1;

    PIC_sendEOI(15);
})

void init_ata()
{
    IRQ_clear_mask(14);
    IRQ_clear_mask(15);

    ata_drive *drive = get_ata_drive(0);

    if (drive != NULL)
    {

        uint8_t *buffer = malloc(512);

        read_drive(drive, buffer, 512, 0);

        /* for (int i = 0; i < 512; i++) */
        /* { */
        /*     printf("%p", buffer[i]); */
        /* } */
    }
}
