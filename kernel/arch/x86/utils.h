#ifndef I386_UTILS_H
#define I386_UTILS_H
#include "kernel/types.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t stack_top;
#define kernel_stack_top &stack_top

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %b0, %w1" : : "a"(val), "nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %w1, %b0" : "=a"(ret) : "nd"(port) : "memory");
    return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "d"(port) : "memory");
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "d"(port) : "memory");
    return ret;
}

static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile("outl %0, %1" : : "a"(val), "d"(port) : "memory");
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "d"(port) : "memory");
    return ret;
}

static inline u64 rdtsc()
{

    uint32_t hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi)::"memory");
    __asm__ volatile("rdtsc");
    return ((uint64_t)hi << 32) | lo;
}

static inline void io_wait(void)
{
    outb(0x80, 0);
}

static inline void irq_enable()
{
    asm("sti");
}

static inline void irq_disable()
{
    asm("cli");
}

#ifdef CONFIG_x86_64
static inline ptr save_eflags(void)
{
    ptr flags;
    __asm__ volatile("pushfq\n\t"
                     "pop %0"
                     : "=r"(flags)
                     :
                     : "memory");
    return flags;
}
static inline void restore_eflags(ptr flags)
{
    __asm__ volatile("push %0\n\t"
                     "popfq"
                     :
                     : "r"(flags)
                     : "memory", "cc");
}
#elif defined(CONFIG_x86_32)

static inline ptr save_eflags(void)
{
    ptr flags;
    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=r"(flags)
                     :
                     : "memory");
    return flags;
}
static inline void restore_eflags(ptr flags)
{
    __asm__ volatile("push %0\n\t"
                     "popf"
                     :
                     : "r"(flags)
                     : "memory", "cc");
}

#endif

typedef bool irq_state_t;
static inline irq_state_t irq_save()
{
    ptr flags = save_eflags();
    ptr is_set = (flags & (1 << 9)) != 0;
    irq_state_t irq_state = (bool)(is_set);
    restore_eflags(flags);

    return irq_state;
}

/**
 * wait for an irq
 *
 * (this is the "hlt" instruction on x86)
 */
static inline void irq_wait()
{
    asm("hlt");
}

/*
 * helper function that disables irqs and saves the state
 * equivalent to:
 * 		irq_save()
 * 		irq_disable()
 */
static inline irq_state_t irq_save_disable()
{
    irq_state_t out = irq_save();
    irq_disable();
    return out;
}

static inline void irq_restore(irq_state_t state)
{
    if (state)
    {
        irq_enable();
    }
    else
    {
        irq_disable();
    }
}

#define IRQ_OFF(block)                                                         \
    {                                                                          \
        irq_state_t state = irq_save_disable();                                \
        block irq_restore(state);                                              \
    }

#endif
