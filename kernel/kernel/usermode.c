#include "kernel/usermode.h"
#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void jump_usermode_asm();

void usermode_main()
{
    /* printf("usermode!!\n"); */
    while (1)
    {
    }
}

void jump_usermode()
{

#define USER_STACK_BASE 0xBFFF0000
#define USER_STACK_SIZE 0x4000
#define USER_STACK_PHYSICAL 0x1000000

    for (uint32_t i = USER_STACK_BASE - USER_STACK_SIZE; i < USER_STACK_BASE;

         i += PAGE_SIZE)
    {

        /* printf("%p\n", (void *)i); */
        map_page((void *)USER_STACK_PHYSICAL +
                     (i - (USER_STACK_BASE - USER_STACK_SIZE)),
                 (void *)i, PAGING_WRITEABLE | PAGING_USER);
    }

// #define USER_CODE_START 0x08048000
#define USER_CODE_START 0x08048000
#define USER_CODE_SIZE 0x3000
    // #define USER_CODE_PHYSICAL 0x1000000
#define USER_CODE_PHYSICAL 0x300000
    for (int i = USER_CODE_START; i < USER_CODE_START + USER_CODE_SIZE;
         i += PAGE_SIZE)
    {
        /* printf("virtual  :%p\n", (void *)i); */
        /* printf("physical :%p\n", */
        /* (void *)USER_CODE_PHYSICAL + (i - USER_CODE_START)); */
        map_page((void *)USER_CODE_PHYSICAL + (i - USER_CODE_START), (void *)i,
                 PAGING_WRITEABLE | PAGING_USER);
    }

    memcpy((void *)USER_CODE_START, usermode_main, USER_CODE_SIZE);

    jump_usermode_asm();
}
