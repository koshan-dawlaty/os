#ifndef x86_PAGING_H
#define x86_PAGING_H

static inline void invlpg(void *addr)
{
    asm volatile("invlpg (%0)" : : "b"(addr) : "memory");
}

#endif
