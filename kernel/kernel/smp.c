#include "../acpi/acpica/acpi.h"
#include "kernel/log.h"
#include "kernel/multiboot.h"
#include "memory/paging_flags.h"
#include "panic.h"
#include "utils.h"
#include <compiler.h>
#include <kernel/paging.h>
#include <stdio.h>
#include <types.h>
#include <unistd.h>

struct local_apic
{
    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags;
} __packed;

struct io_apic
{
    u8 apic_id;
    u8 reserved;
    u32 apic_addr;
    u32 global_interrupt_base;
} __packed;

ptr lapic;
#define OFFSET_LAPIC(offset) ((volatile uint32_t *)(lapic + offset))
size_t num_cores;
size_t aprunning;
bool bspdone;

struct local_apic local_apics[16];

extern void ap_trampoline();

void hi()
{
    printf("hi\n");
    while (1)
    {
    }
}

void bootstrap_init()
{
    u8 bspid;
    asm volatile("mov $1, %%eax; cpuid; shrl $24, %%ebx;" : "=b"(bspid) : :);

    /* map bootstrap code */
    map_page((void *)0x8000, (void *)0x8000, PAGING_WRITEABLE | PAGING_PRESENT);
    map_page((void *)0x9000, (void *)0x9000, PAGING_WRITEABLE | PAGING_PRESENT);
    memcpy((void *)0x8000, &ap_trampoline, 4096);
    /* map the 32 bit part of the bootstrap code */
    map_page((void *)0x100000, (void *)0x100000,
             PAGING_PRESENT | PAGING_WRITEABLE);

    for (size_t i = 0; i < num_cores; i++)
    {
        /* https://wiki.osdev.org/Symmetric_Multiprocessing */
        if (local_apics[i].apic_id == bspid)
        {
            continue;
        }

        *OFFSET_LAPIC(0x310) = *OFFSET_LAPIC(0x310) | (i << 24); /* select ap */
        *OFFSET_LAPIC(0x300) = (*(OFFSET_LAPIC(0x300)) & 0xfff00000) | 0x00C500;
        do
        {
            asm volatile("pause" : : : "memory");
        } while (*OFFSET_LAPIC(0x300) & (1 << 12)); /* wait for delivery */
        *(OFFSET_LAPIC(0x310)) =
            (*(OFFSET_LAPIC(0x310))) | (i << 24); /* select ap */
        *(OFFSET_LAPIC(0x300)) = (*(OFFSET_LAPIC(0x300)) & 0xfff00000) |
                                 0x008500; /* init level deassert, tell the ap
                                              to start running code */
        do
        {
            asm volatile("pause" : : : "memory");
        } while (*(OFFSET_LAPIC(0x300)) & (1 << 12)); /* wait for delivery */
        /* sleep(100000); */
        /* TODO: the PIT can be polled here instead to make the busy wait use
         * less power */
        /* wait (maybe) 10 miliseconds */
        for (size_t i = 0; i < 10000; i++)
        {
            io_wait();
        }
        for (size_t j = 0; j < 2; j++)
        {
            *OFFSET_LAPIC(0x280) = 0; /* clear APIC errors */
            *OFFSET_LAPIC(0x310) =
                (*(OFFSET_LAPIC(0x310))) | (i << 24); /* select AP */
            *OFFSET_LAPIC(0x300) =
                (*OFFSET_LAPIC(0x300) & 0xfff0f800) |
                0x000608; /* trigger STARTUP IPI for 0800:0000 */

            /* wait (potentially) 200 usec */
            for (size_t i = 0; i < 200; i++)
            {
                io_wait();
            }
            do
            {
                asm volatile("pause" : : : "memory");
            } while (*((volatile uint32_t *)(OFFSET_LAPIC(0x300))) &
                     (1 << 12)); /* wait for delivery */
        }
        bspdone = true;
    }
}

void smp_init()
{
    ACPI_STATUS status;

    ACPI_TABLE_HEADER *table;

    status = AcpiGetTable(ACPI_SIG_MADT, 0, &table);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI didnt find madt");
    }
    ACPI_TABLE_MADT *madt = (ACPI_TABLE_MADT *)table;
    ACPI_SUBTABLE_HEADER *entry;
    u8 *end = (u8 *)madt + madt->Header.Length;

    u32 lapic_addr = madt->Address;
    void *lapic_physical = (void *)(ptr)lapic_addr;
    lapic = (ptr)map_physaddr(lapic_physical, 1, PAGING_WRITEABLE);
    logd("lapic:%p", lapic_addr);
    entry = (ACPI_SUBTABLE_HEADER *)(madt + 1);

    /* entry += entry->Length; */

    while ((u8 *)entry < end)
    {
        switch (entry->Type)
        {
        case ACPI_MADT_TYPE_LOCAL_APIC:
            printf("%d\n", entry->Length);
            printf("found an local apic\n");
            struct local_apic *local_ptr = (struct local_apic *)(entry + 1);
            local_apics[num_cores] = *local_ptr;
            num_cores++;

            printf("apic id:%d\n", local_ptr->apic_id);

            break;

        case ACPI_MADT_TYPE_IO_APIC:
            printf("found the ioapic\n");
            struct io_apic *io_ptr = (struct io_apic *)(entry + 1);
            printf("addr:%p\n", io_ptr->apic_addr);
            u32 *virtual = map_physaddr((void *)(ptr)io_ptr->apic_addr, 1,
                                        PAGING_WRITEABLE | PAGING_PRESENT);
            printf("read this:%p\n", *virtual);

            break;

        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            break;
        }

        entry = (ACPI_SUBTABLE_HEADER *)((u8 *)entry + entry->Length);
    }

    bootstrap_init();
}
