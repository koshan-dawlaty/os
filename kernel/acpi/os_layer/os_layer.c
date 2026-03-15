#include "../os_layer.h"
#include "assert.h"
#include "kernel/log.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "memory/get_free_pages.h"
#include "memory/paging_flags.h"
#include "scheduler.h"
#include "sys/types.h"
#include "utils.h"
#include <acpi.h>
#include <ctype.h>
#include <panic.h>
#include <spinlock.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void AcpiOsVprintf(const char *format, va_list args)
{
    /* vlog_level(LOG_DEBUG, format, args); */
    /* vprintf(format, args); */
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...)
{

    va_list parameters;
    va_start(parameters, Format);
    /* vlog_level(LOG_DEBUG, Format, parameters); */
    /* vprintf(Format, parameters); */
    /* printf("%s\n", Format); */

    va_end(parameters);
}

void strcpy(char *dest, const char *src)
{
    UNREACHABLE("not implemented");
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}
int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

int memcmp(const void *s1, const void *s2, size_t n)
{

    const u8 *one = s1;
    const u8 *two = s2;
    for (size_t i = 0; i < n; i++)
    {

        if (one[i] != two[i])
        {
            return (int)one[i] - (int)two[i];
        }
    }

    return 0;
}

/* const unsigned short int **__ctype_b_loc() */
/* { */
/*     return NULL; */
/* } */

struct RSDP
{
    char Signature[8]; /* "RSD PTR " */
    u8 Checksum;
    char OEMID[6];
    u8 Revision;
    u32 RsdtAddress;
};

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{

    ACPI_PHYSICAL_ADDRESS ret = 0;
    for (size_t i = 0x000E0000; i < 0x000FFFFF + PAGE_SIZE; i += PAGE_SIZE)
    {
        map_page((void *)(i), (void *)i, PAGING_WRITEABLE | PAGING_PRESENT);
    }
    for (ptr addr = 0xE0000; addr < 0x100000; addr += 16)
    {
        if (memcmp((void *)addr, "RSD PTR ", 8) == 0)
        {
            printf("RSDP found at %x\n", addr);
            u8 sum = 0;
            for (int i = 0; i < 20; i++)
                sum += ((u8 *)addr)[i];

            printf("checksum:%d\n", sum);
            ret = addr;
        }
    }

    /* AcpiFindRootPointer(&ret); */
    logd("rsdp:%p", ret);
    return ret;
}

void AcpiOsWaitEventsComplete()
{
    UNREACHABLE("not implemented");
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
                                        UINT64 Value, UINT32 Width)
{
    UNREACHABLE("not implemented");
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                ACPI_TABLE_HEADER **NewTable)
{
    /* UNREACHABLE("not implemented"); */
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                        ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength)
{
    UNREACHABLE("not implemented");
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
                                       UINT64 *Value, UINT32 Width)
{
    UNREACHABLE("not implemented");
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue,
                             UINT32 RegbValue)
{
    UNREACHABLE("not implemented");
    /* this is called right before entering acpi sleep */
    return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId()
{

    /* TODO: this only works when one task is being run */
    return 1;
}

UINT64 AcpiOsGetTimer()
{
    UNREACHABLE("not implemented");
    return 0;
}

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

    void *physical = (void *)(uintptr_t)PhysicalAddress;

    for (size_t i = 0; i < amount; i++)
    {
        logd("%p, %p", physical + (i * PAGE_SIZE), pages + (i * PAGE_SIZE));
        map_page(physical + (i * PAGE_SIZE), pages + (i * PAGE_SIZE),
                 PAGING_WRITEABLE | PAGING_PRESENT);
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
    UNREACHABLE("not implemented");
    return true;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length)
{
    /* TODO: this does not seem to get called*/
    UNREACHABLE("not implemented");
    return true;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
                                     ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{

    UNREACHABLE("not implemented");
    *PhysicalAddress = (ptr)get_physaddr(LogicalAddress);

    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE size)
{
    void *out = malloc(size);
    bzero(out, size);
    return out;
}

void AcpiOsFree(void *Memory)
{
    free(Memory);
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
                          ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
    UNREACHABLE(
        "probably broken, because AcpiOsGetThreadId() is not implemented");
    ktask_run(Function, Context);
    return AE_OK;
}

/* TODO: this does not sleep for the correct number of milliseconds, however it
 * is always more than what it wants */
void AcpiOsSleep(UINT64 Milliseconds)
{
    UNREACHABLE("probably does not work with inital kernel task");
    sleep(Milliseconds);
}

/* TODO: this does not sleep for the correct number of microseconds and it might
 * be less than what it wants */
void AcpiOsStall(UINT32 Microseconds)
{
    UNREACHABLE("probably does not work");
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
    UNREACHABLE("not implemented");
    return AE_OK;
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
    *NewValue = NULL;
    return AE_OK;
}

/* used to recive signals from acpica */
ACPI_STATUS
AcpiOsSignal(UINT32 Function, void *Info)
{
    return AE_OK;
}
