#ifndef KERNEL_FAULTS_H
#define KERNEL_FAULTS_H

#include <stdbool.h>

struct page_fault_flags {
    bool user : 1;
    bool instruction : 1;
};

void page_fault(void * addr, struct page_fault_flags flags);

#endif