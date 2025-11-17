#include "../os_layer.h"
#include "kernel/paging.h"
#include "kernel/utils.h"
#include "memory/get_free_pages.h"
#include "memory/paging_flags.h"
#include <acpi.h>
#include <stdint.h>
#include <stdlib.h>

ACPI_STATUS AcpiOsInitialize()
{
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate()
{
    return AE_OK;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
    /* TODO: this should be in its own function for DMA */
    size_t len = align_long(Length + 4096, 4096);
    size_t amount = len / 4096;
    void *pages = get_free_pages_unmmaped(amount, &gfp_kernel_zone);
    /* TODO: this is not good, it can run into other get free pages zones,
     * we should just get a memory map from the bios at some point */
    void *physical = (void *)(uintptr_t)PhysicalAddress;

    for (size_t i = 0; i < amount; i++)
    {
        map_page(physical + (i * PAGE_SIZE), pages + (i * PAGE_SIZE),
                 PAGING_USER);
    }

    return pages + PhysicalAddress % 4096;
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length)
{
    /* TODO */
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length)
{
    /* TODO: shouldnt affect anything that much */
    return true;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length)
{
    /* TODO: this does not seem to get called*/
    return true;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
                                     ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{

    *PhysicalAddress = (ptr)get_physaddr(LogicalAddress);

    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
    return malloc(Size);
}

void AcpiOsFree(void *Memory)
{
    free(Memory);
}
