#include <container_of.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/compiler/codegen_test123.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>

void ir_test()
{

    const char *expr_str = MACRO_STR(int main() {
        /* return 5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9 + 2 + 3 * 4 - 6 * 2 + 7
         * + */
        /*        8 * 2 - 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8 + */
        /* return 10 * (2 + 3 * (4 + 5)); */
        /* return 2 * 3; */
        /* return (8 * 2) - (9 * 3); */
        int ptr = 40;

        *ptr = 123;

        return *ptr;
    }

    );

    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(expr_str, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);
    struct ast_top_statement *main =
        containerof(top_statements.next, struct ast_top_statement, list);

    struct ir_program program = {.register_offset = 0, 0, {0}, {0}};
    list_init_head(&program.instructions);

    ir_generate_block(&program, &main->function->block);
    ir_display(&program, &LOG_INFO);

    /* struct cfg_parser cfg_parser = {0}; */
    /* struct cfg_node *root = */
    /*     cfg_parse(&cfg_parser, &program, -1, program.instructions.next); */
    /* cfg_display(root); */
    /* logd("dominance"); */
    /* cfg_set_dominance(&cfg_parser, root); */
    /* logd("dominance frontier"); */
    /* cfg_set_dominance_frontier(&cfg_parser); */

    struct bytecode_array code = {0};
    test123_generate_ir(&code, &program);

    log_multiline_start(&LOG_DEBUG);
    logd("\n");
    for (size_t i = 0; i < code.num; i += 4)
    {
        logd("%p\n", *(u32 *)(code.items + i));
        /* logd("%p%p%p%p\n", code.items[i], code.items[i + 1], code.items[i +
         * 2], */
        /*      code.items[i + 3]); */
    }
    logd("\n");
    /*
for (size_t i = 0; i < code.num; i++)
{
    if (code.items[i] < 16)
    {
        logd("0%p ", code.items[i]);
    }
    else
    {
        logd("%p ", code.items[i]);
    }
}
logd("\n");
    */
    log_multiline_end(&LOG_DEBUG);

    /* run_code(code.items, code.num); */
}

int main()
{

    ir_test();
    return 0;

    LIST_DEFINE_HEAD(tokens);
    const char *program = MACRO_STR(
        // clang-format off

	int main() {
		int x = 20;
		int y = 10;
		int z = x - y * 2;

		if (z == 0) {
			return 1;
		}

		return 0;
	}

        // clang-format on
    );

    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements_head);

    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_error *err = ast_parse(&tokens, &top_statements_head);
    if (err != NULL)
    {
        static char buf[100];
        ast_error_to_string(err, buf, 100);
        PANIC("error:%s", buf);
    }

    struct bytecode_array code = {0};
    list_init_head(&code.current_scope_vars);

    struct ast_top_statement *function =
        containerof(top_statements_head.next, struct ast_top_statement, list);
    ASSERT(function->type == AST_TOP_STATEMENT_FUNC,
           "not implemented for not function");
    generate_function(&code, function);

    /* code = (struct bytecode_array){0, 0, 0}; */

    /* struct codegen_value *stack = generate_push( */
    /*     &code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0});
     */
    /* /1* generate_mov(&code, stack, *1/ */
    /* /1*              &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE,
     * 0x12}); *1/ */
    /* generate_sub(&code, stack, */
    /*              &(struct codegen_value){CODEGEN_VALUE_REGISTER,
     * 0b110}); */
    /* array_macro_append_v(&code, 0xc3); */

    /* generate_add(&code, &(struct
     * codegen_value){CODEGEN_VALUE_REGISTER, 0b1000}, */
    /*              &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE,
     * 1}); */

    log_multiline_start(&LOG_DEBUG);
    for (size_t i = 0; i < code.num; i++)
    {
        if (code.items[i] < 16)
        {
            logd("0%p ", code.items[i]);
        }
        else
        {
            logd("%p ", code.items[i]);
        }
    }
    log_multiline_end(&LOG_DEBUG);

    run_code(code.items, code.num);
}
