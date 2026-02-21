#ifndef KERNEL_FAULTS_H
#define KERNEL_FAULTS_H

#include <stdbool.h>

struct page_fault_flags
{
    bool not_present : 1;
    bool user : 1;
    bool instruction : 1;
    bool other : 1;
};

void page_fault(void *addr, void *ip, struct page_fault_flags flags);

#endif
