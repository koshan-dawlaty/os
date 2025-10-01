#include "kernel/types.h"
#include <stdint.h>

const u32 CPUID_FLAG_MSR = 1 << 5;

bool cpuHasMSR()
{
    static u32 a, d;
    /* cpuid(1, &a, &d); */
    return d & CPUID_FLAG_MSR;
}

void cpuGetMSR(u32 msr, u32 *lo, u32 *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void cpuSetMSR(u32 msr, u32 lo, u32 hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
