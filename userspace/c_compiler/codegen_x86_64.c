#include "c_compiler.h"
#include <init.h>
#include <lib/list.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <utils.h>

extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);

int main()
{

    LIST_DEFINE_HEAD(tokens);
    const char *program = MACRO_STR(
        // clang-format off
	int main() {
		return 0;
	}

        // clang-format on
    );
    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements_head);
    ast_parse(&tokens, &top_statements_head);

    u8 bytecode[] = {0xa3, 0x34, 0x12, 0, 0, 0, 0, 0, 0};
    logd("size:%d", sizeof(bytecode));

    u8 *binary_test = malloc(9 + sizeof(bytecode));
    binary_test[0] = 'T';
    binary_test[1] = 'E';
    binary_test[2] = 'S';
    binary_test[3] = 'T';
    binary_test[4] = 0;
    *(u32 *)(binary_test + 5) = sizeof(bytecode);
    memcpy(binary_test + 9, bytecode, sizeof(bytecode));
    syscall_wrapper_asm(SYS_load_simple_binary, (ptr_t)binary_test, 0, 0, 0, 0);

    exit(0);
    return 0;
}
