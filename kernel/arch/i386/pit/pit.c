#include "pit.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "utils.h"
#include <scheduler.h>
#include <stdint.h>
#include <stdio.h>

volatile uint64_t pit_time = 0;

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

#define PIT_REAL_FREQUENCY 1193180

// should not be lower than 19
#define PIT_FREQUENCY 100

void init_pit()
{
    int divisor = PIT_REAL_FREQUENCY / PIT_FREQUENCY;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);

    /* outb(PIT_CHANNEL0, 0); */
}

void pit_handler_c(struct registers *r)
{
    /* printf("pit\n"); */

    pit_time++;
    if (pit_time % 50 == 0)
    {
        /* printf("task switch ip:%p\n", r->frame.ip); */

        // check if it is in usermode
        if ((r->frame.cs & 3) == 3)
        {

            switch_tasks(*r);
        }

        /* printf("%p\n", frame->ip); */
        /* printf("pit time:%d\n", get_pit_time()); */
    }

    PIC_sendEOI(0);
}

ISR(pit_handler, {
    /* printf("seconds:%d\n", get_seconds()); */
    printf("pit\n");

    /*     pit_time++; */
    /*     if (pit_time % 50 == 0) */
    /*     { */
    /*         /1* printf("task switch ip:%p\n", frame->ip); *1/ */

    /*         printf("sp:%p\n", frame->sp); */
    /*         /1* switch_tasks(frame->ip, frame->sp); *1/ */

    /*         /1* printf("%p\n", frame->ip); *1/ */
    /*         /1* printf("pit time:%d\n", get_pit_time()); *1/ */
    /*     } */

    PIC_sendEOI(0);
});

uint64_t get_seconds()
{
    return pit_time / PIT_FREQUENCY;
}

uint64_t get_pit_time()
{
    return pit_time;
}
