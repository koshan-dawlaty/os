#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/utils.h>

int main()
{

    LIST_DEFINE_HEAD(tokens);
    const char *program = MACRO_STR(
        // clang-format off

	int main() {
		return 
                    5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9
                  + 2 + 3 * 4 - 6 * 2 + 5 + 7 + 8 * 2
                  - 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8
                  + 10 * (2 + 3 * (4 + 5));
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
