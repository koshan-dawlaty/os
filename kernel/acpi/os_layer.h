#ifndef ACPICA_OS_LAYER
#define ACPICA_OS_LAYER
#include <stddef.h>
#include <stdint.h>

#define ACPI_SINGLE_THREADED 1
// #define ACPI_NO_ACPI_ENABLE 1
#define ACPI_REDUCED_HARDWARE 1
#define ACPI_NO_EVENT_SUPPORT 1
#define ACPI_HW_ACCESS_NONE 1

#define ACPI_USE_STANDARD_HEADERS 0

#ifdef CONFIG_x86_32
#define ACPI_MACHINE_WIDTH 32
#elif defined(CONFIG_x86_64)
#define ACPI_MACHINE_WIDTH 64
#endif

// #undef ACPI_USE_SYSTEM_CLIBRARY

#define ACPI_USE_LOCAL_CACHE
#define ACPI_USE_NATIVE_DIVIDE

#define ACPI_USE_SYSTEM_CLIBRARY 0
#define ACPI_USE_LOCAL_CACHE
#define ACPI_USE_DEFAULT_ENV
#define ACPI_USE_DEFAULT_ALLOCATES
#define ACPI_USE_NATIVE_DIVIDE

/* from
 * https://www.intel.com/content/www/us/en/developer/topic-technology/open/acpica/documentation.html
 */
#ifdef CONFIG_x86_32
#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32)                           \
    u32 _q, _r;                                                                \
    __asm__ volatile("divl %4"                                                 \
                     : "=a"(_q), "=d"(_r)                                      \
                     : "a"(n_lo), "d"(n_hi), "r"(d32));                        \
    (q32) = _q;                                                                \
    (r32) = _r;
#else
#define ACPI_DIV_64_BY_32(n, n_hi, n_lo, d32, q32, r32)                        \
    {                                                                          \
        q32 = n / d32;                                                         \
        r32 = n % d32;                                                         \
    }
#endif

#endif
