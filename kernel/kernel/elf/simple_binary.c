#include "kernel/addr_space.h"
#include "kernel/userspace.h"
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <lib/log.h>
#include <scheduler.h>
#include <sys/syscall.h>
#include <vfs.h>

/* FIXME: stupid */
extern void user_thread_kernel_start(ptr_t addr);

struct task *simple_binary_load(u8 *data)
{
    if (data[0] != 'T' || data[1] != 'E' || data[2] != 'S' || data[3] != 'T' ||
        data[4] != 0)
    {
        PANIC("invalid simple binary");
    }

    u32 bin_len = *(u32 *)(data + 5);
    void *kernel = malloc(9 + bin_len);
    logd("%p %d", kernel, bin_len);
    copy_from_user(kernel, data, bin_len + 9);
    struct addr_space *current = get_current_addr_space();

    struct task *out = new_task_user(0, 0xBFFEF000, 0, 0);
    addr_space_load(&out->addr_space);
    logd("before gfp_user");
    void *memory = get_free_pages_user((bin_len / PAGE_SIZE) + 1);
    logd("kernel:%p, new task:%p", kernel, memory);
    out->start_addr = memory;
    out->inital_arg = (ptr_t)memory;
    out->ran_setup = false;
    /* out->regs.edx = (ptr_t)memory; */
    memcpy(memory, kernel + 9, bin_len);
    *((ptr_t *)(out->stack)) = (ptr_t)user_thread_kernel_start;

    addr_space_load(current);
    create_userspace_stack(&out->addr_space);

    return out;
}

SYSCALL_DEFINE1(SYS_load_simple_binary, int, u8 *, data)
{
    add_task(simple_binary_load(data));
    return 0;
}
