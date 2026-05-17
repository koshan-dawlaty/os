#include <kernel/multiboot.h>
#include <lib/array.h>
#include <lib/log.h>
#include <stddef.h>

struct symbol_table_ent
{
    u64 addr; /* this is always 64 bit */
    char *name;
};
struct symbol_table_ent *symbol_table;
size_t symbol_table_len;

struct bootloader_module bootloader_symbol_table;

#define SYMBOL_TABLE_END "____SYMTABLE_DONE"

void symbol_table_parse()
{
    struct array a = ARRAY_INIT();
    size_t offset = 0;
    char *ptr = (char *)bootloader_symbol_table.addr;
    size_t num = 0;
    while (true)
    {
        if (!strcmp(ptr, SYMBOL_TABLE_END))
        {
            break;
        }

        char *n = strdup((ptr + sizeof(u64)));
        struct symbol_table_ent ent = {.addr = *((u64 *)ptr), .name = n};

        ptr += strlen(n) + 1 + sizeof(u64);
        array_push(&a, &ent, sizeof(struct symbol_table_ent));
        num++;
    }

    symbol_table_len = num;

    /* TODO: free bootloader_symbol_table */
    symbol_table = array_free_ptr(a);
}

/*
 * you dont own the returned pointer
 * returns null if @addr is not in the table
 */
char *symbol_get_name(ptr addr)
{
    for (size_t i = 0; i < symbol_table_len; i++)
    {
        if (symbol_table[i].addr < addr)
        {
            return symbol_table[i].name;
        }
    }

    return NULL;
}

size_t max_frames = 10;
struct stackframe
{
    struct stackframe *bp;
    ptr ip;
};
void stacktrace_log()
{
    struct stackframe *stk;
    asm("mov %%rbp,%0" : "=r"(stk)::);
    loge("stack trace:");
    for (unsigned int frame = 0; stk && frame < max_frames; ++frame)
    {
        loge("0x%p, %s", stk->ip, symbol_get_name(stk->ip));
        stk = stk->bp;
    }
}
