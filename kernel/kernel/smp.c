#include "../acpi/acpica/acpi.h"
#include "interrupts/pic.h"
#include "kernel/log.h"
#include "kernel/multiboot.h"
#include "kernel/types.h"
#include "memory/get_free_pages.h"
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

struct acpi_io_apic
{
    u8 apic_id;
    u8 reserved;
    u32 apic_addr;
    u32 global_interrupt_base;
} __packed;

struct io_apic
{
    u8 version;
    u8 max_irqs;
    ptr addr;
    u32 global_interrupt_base;
};

ptr lapic;
#define OFFSET_LAPIC(offset) ((volatile uint32_t *)(lapic + offset))
size_t num_cores;
size_t aprunning;
bool bspdone;

struct local_apic local_apics[16];

void ap_started()
{
    printf("a\n");
    irq_enable();
    while (1)
    {
        irq_wait();
    }
}

extern void ap_trampoline();
extern void ap_64();
/* used by ap_init.S */
void **ap_stacks;

void bootstrap_init()
{
    u8 bspid;
    asm volatile("mov $1, %%eax; cpuid; shrl $24, %%ebx;" : "=b"(bspid) : :);

    /* map bootstrap code */
    map_page((void *)0x8000, (void *)0x8000, PAGING_WRITEABLE | PAGING_PRESENT);
    map_page((void *)0x9000, (void *)0x9000, PAGING_WRITEABLE | PAGING_PRESENT);
    memcpy((void *)0x8000, &ap_trampoline, 4096);
    memcpy((void *)0x9000, &ap_64, 4096);
    // *(u8 *)(0x9000) = 0xF4;
    u8 apic_max = local_apics[0].apic_id;
    for (size_t i = 0; i < num_cores; i++)
    {
        u8 id = local_apics[i].apic_id;
        if (id > apic_max)
        {
            apic_max = id;
        }
    }
    /* this is probably redundant, but it might prevent a bug */
    if (apic_max != num_cores - 1)
    {
        panic("max apic id:%d not equal to number of apics:%d", apic_max,
              num_cores - 1);
    }

    ap_stacks = malloc(sizeof(void *) * num_cores - 1);
    for (size_t i = 0; i < num_cores - 1; i++)
    {
        /* logd("writing a stack to:%d", i); */
        ap_stacks[i] = get_free_pages(2) + ((PAGE_SIZE * 2) - 16);
    }

    for (size_t i = 0; i < num_cores; i++)
    {
        /* https://wiki.osdev.org/Symmetric_Multiprocessing */
        if (local_apics[i].apic_id == bspid)
        {
            continue;
        }

        u8 id = local_apics[i].apic_id;

        *OFFSET_LAPIC(0x310) = (id << 24); /* select ap */
        *OFFSET_LAPIC(0x300) = (*(OFFSET_LAPIC(0x300)) & 0xfff00000) | 0x00C500;
        do
        {
            asm volatile("pause" : : : "memory");
        } while (*OFFSET_LAPIC(0x300) & (1 << 12)); /* wait for delivery */
        *(OFFSET_LAPIC(0x310)) = (id << 24); /* select ap */
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
            *OFFSET_LAPIC(0x280) = 0;          /* clear APIC errors */
            *OFFSET_LAPIC(0x310) = (id << 24); /* select AP */
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
    }

    bspdone = true;
}

u32 ioapic_read(ptr base, ptr offset)
{
    *(volatile u32 *)(base) = offset;
    return *(volatile u32 *)(base + 0x10);
}

void ioapic_write(ptr base, ptr offset, u32 value)
{
    *(volatile u32 *)(base) = offset;
    *(volatile u32 *)(base + 0x10) = value;
}

struct io_apic io_apic;

union redirection_entry
{
    struct
    {
        /* TODO: check if this bitfield order is consistent */
        u64 vector : 8;
        u64 delv_mode : 3;
        u64 dest_mode : 1;
        u64 delv_status : 1;
        u64 pin_polarity : 1;
        u64 remote_irr : 1;
        u64 trigger_mode : 1;
        u64 mask : 1;
        u64 reserved : 39;
        u64 destination : 8;
    } __packed;
    struct
    {
        u32 low;
        u32 high;
    } __packed;
};

union redirection_entry ioapic_get_redir_entry(struct io_apic apic, u8 irq)
{
    if (irq > apic.max_irqs)
    {
        logd("irq %d too high", irq);
    }
    size_t offset = 0x10 + 2 * irq;
    u32 low = ioapic_read(apic.addr, offset);
    u32 high = ioapic_read(apic.addr, offset + 1);
    return (union redirection_entry){.high = high, .low = low};
}

void ioapic_write_redir_entry(struct io_apic apic, u8 irq,
                              union redirection_entry ent)
{
    if (irq > apic.max_irqs)
    {
        logd("irq %d too high", irq);
    }
    size_t offset = 0x10 + 2 * irq;
    ioapic_write(apic.addr, offset, ent.low);
    ioapic_write(apic.addr, offset + 1, ent.high);
}

void smp_acpi_init()
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
        case ACPI_MADT_TYPE_LOCAL_APIC:;
            /* printf("%d\n", entry->Length); */
            /* printf("found an local apic\n"); */
            struct local_apic *local_ptr = (struct local_apic *)(entry + 1);
            local_apics[num_cores] = *local_ptr;
            num_cores++;

            /* printf("apic id:%d\n", local_ptr->apic_id); */

            break;

        case ACPI_MADT_TYPE_IO_APIC:;
            /* printf("found the ioapic\n"); */
            struct acpi_io_apic *io_ptr = (struct acpi_io_apic *)(entry + 1);
            /* printf("addr:%p\n", io_ptr->apic_addr); */
            ptr ioapic_base =
                (ptr)map_physaddr((void *)(ptr)io_ptr->apic_addr, 1,
                                  PAGING_WRITEABLE | PAGING_PRESENT);

            u32 apic_version = ioapic_read(ioapic_base, 1);
            u8 version = apic_version & 0xFF;
            u8 max_irqs = (apic_version >> 16) + 1;

            io_apic = (struct io_apic){
                .version = version,
                .max_irqs = max_irqs,
                .addr = ioapic_base,
                .global_interrupt_base = io_ptr->global_interrupt_base,
            };

            break;

        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            break;
        }

        entry = (ACPI_SUBTABLE_HEADER *)((u8 *)entry + entry->Length);
    }
}

void smp_init()
{
    smp_acpi_init();

    /* for (size_t i = 0; i < io_apic.max_irqs; i++) */
    /* { */
    /*     union redirection_entry ent = ioapic_redir_entry(io_apic, i); */
    /*     printf("%p\n", ent.mask); */
    /* } */

    bootstrap_init();

    /* PIC_mask_all(); */

    /* union redirection_entry ent = ioapic_get_redir_entry(io_apic, 0); */
    /* ent.mask = 0; */
    /* ent.vector = 32; */
    /* ent.delv_mode = 0; */
    /* ent.pin_polarity = 0; */
    /* ent.trigger_mode = 0; */
    /* ent.dest_mode = 0; */
    /* ent.destination = 1; */
    /* ioapic_write_redir_entry(io_apic, 0, ent); */
}
