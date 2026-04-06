#ifndef MSR_H
#define MSR_H

#include <kernel/types.h>
#include <stdint.h>

const u32 CPUID_FLAG_MSR = 1 << 5;

static inline void cpuid(int code, u32 *a, u32 *d)
{
    asm volatile("cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx");
}

static inline bool cpu_has_msr()
{
    static u32 a, d;
    cpuid(1, &a, &d);
    return d & CPUID_FLAG_MSR;
}

static inline void cpu_get_msr(u32 msr, u32 *lo, u32 *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline void cpu_set_msr(u32 msr, u32 lo, u32 hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

#endif
