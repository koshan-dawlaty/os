#ifndef IDT_H
#define IDT_H
#include <stdint.h>
#include <types.h>

void init_idt();

struct interrupt_frame
{
#ifdef CONFIG_x86_64
    u64 ip, cs, flags, sp, ss;
#else
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
#endif
} __attribute__((packed));

#define ISR_ATTRIBUTE __attribute__((interrupt, target("general-regs-only")))

/*
 * defines a function called name, with a parameter of type interrupt_frame *
 * called frame.
 */
#define ISR(name, body)                                                        \
    _Pragma("clang diagnostic push");                                          \
    _Pragma("clang diagnostic ignored \"-Wexcessive-regsave\"");               \
    ISR_ATTRIBUTE void name(struct interrupt_frame *frame)                     \
    {                                                                          \
        /* this is to prevent the unused parameter warning */                  \
        (void)frame;                                                           \
        body                                                                   \
    }                                                                          \
    _Pragma("clang diagnostic pop");

#endif
