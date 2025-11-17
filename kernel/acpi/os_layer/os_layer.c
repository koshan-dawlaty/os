#include "../os_layer.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/utils.h"
#include "memory/get_free_pages.h"
#include "memory/paging_flags.h"
#include "scheduler.h"
#include "utils.h"
#include <acpi.h>
#include <spinlock.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
                          ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
    run_task(Function, Context);
    return AE_OK;
}

/* TODO: this does not sleep for the correct number of milliseconds, however it
 * is always more than what it wants */
void AcpiOsSleep(UINT64 Milliseconds)
{
    sleep(Milliseconds);
}

/* TODO: this does not sleep for the correct number of microseconds and it might
 * be less than what it wants */
void AcpiOsStall(UINT32 Microseconds)
{
    for (size_t i = 0; i < Microseconds; i++)
    {
        io_wait();
    }
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits,
                                  ACPI_SEMAPHORE *OutHandle)
{
    /* TODO: maybe use InitialUnits? */
    *OutHandle = new_semaphore(MaxUnits);
    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
    free_semaphore(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units,
                                UINT16 Timeout)
{
    wait_semaphore(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
    release_semaphore(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    *OutHandle = new_spinlock();
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_HANDLE Handle)
{
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    lock_spinlock(Handle);
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
    release_spinlock(Handle);
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel,
                                          ACPI_OSD_HANDLER Handler,
                                          void *Context)
{
    return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...)
{

    va_list parameters;
    va_start(parameters, Format);
    vprintf(Format, parameters);

    va_end(parameters);
}
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
    void *addr = (void *)(ptr)Address;
    switch (Width)
    {
    case 8:
        *Value = *(u8 *)addr;
        break;
    case 16:
        *Value = *(u16 *)addr;
        break;
    case 32:
        *Value = *(u32 *)addr;
        break;
    case 64:
        *Value = *(u64 *)addr;
        break;
    }

    return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
    void *addr = (void *)(ptr)Address;
    switch (Width)
    {
    case 8:
        *(u8 *)addr = Value;
        break;
    case 16:
        *(u16 *)addr = Value;
        break;
    case 32:
        *(u32 *)addr = Value;
        break;
    case 64:
        *(u64 *)addr = Value;
        break;
    }

    return AE_OK;
}
ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{

    u16 port = Address;

    switch (Width)
    {
    case 8:
        outb(port, Value);
        break;
    case 16:
        outw(port, Value);
        break;
    case 32:
        outl(port, Value);
        break;
    }
    return AE_OK;
}

ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
    u16 port = Address;

    switch (Width)
    {
    case 8:
        *Value = inb(port);
        break;
    case 16:
        *Value = inw(port);
        break;
    case 32:
        *Value = inl(port);
        break;
    }
    return AE_OK;
}

/* TODO: no idea what this is */
ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject,
                         ACPI_STRING *NewValue)
{

    return AE_OK;
}

/* used to recive signals from acpica */
ACPI_STATUS
AcpiOsSignal(UINT32 Function, void *Info)
{
    return AE_OK;
}
