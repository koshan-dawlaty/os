#include "acpica/acpi.h"
#include <kernel/paging.h>
#include <panic.h>
#include <stdio.h>
#include <types.h>

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
};

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

    entry = (ACPI_SUBTABLE_HEADER *)(madt + 1);

    while ((u8 *)entry < end)
    {
        switch (entry->Type)
        {
        case ACPI_MADT_TYPE_LOCAL_APIC:
            printf("%d\n", entry->Length);
            printf("found an local apic\n");
            struct local_apic *local_ptr = (struct local_apic *)(entry + 1);

            printf("%d\n", local_ptr->apic_id);

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
}

void acpica_init()
{
    ACPI_STATUS status;
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status))
    {
        panic("ACPI subsystem init failed");
    }

    status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI table init failed");
    }

    status = AcpiLoadTables();
    if (ACPI_FAILURE(status))
    {
        panic("ACPI load tables failed");
    }

    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI enable subsystem failed");
    }

    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI object init failed");
    }

#ifdef CONFIG_SMP
    smp_init();
#endif
}
