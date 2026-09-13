/* TODO: this has to be in userspace for right now, maybe move to kernel tests?
 */

/* this is a test executable, so always run tests */
#ifndef TESTS
#define TESTS
#endif

#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/list.h>
#include <lib/log.h>
#include <tests/tests.h>

struct test_result *test_codegen_expr(const char *program_str, int expected)
{

    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(program_str, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);

    struct ir_program program = {.register_offset = 0, {0}};
    list_init_head(&program.operations);
    struct ast_top_statement *main =
        containerof(top_statements.next, struct ast_top_statement, list);
    ir_generate_block(&program, &main->function->block);

    struct bytecode_array code = {0};
    codegen_generate_ir(&code, &program);
    int got = run_code(code.items, code.num);
    ASSERT_EQ(got, expected, ", %s (%d) != %d", program, expected, got);

    TEST_OK();
}

#define TEST_CODEGEN_EXPR(name, expr, expected)                                \
    TEST(name, { return test_codegen_expr("int main() { return" expr ";}", expected); })

TEST_CODEGEN_EXPR(test_codegen_expr1, "1 + 1", 2);
TEST_CODEGEN_EXPR(test_codegen_expr2, "2 + 3 * 4 - 6 * 2 + 5", 7);
TEST_CODEGEN_EXPR(test_codegen_expr3,
                  "  5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
                  "+ 2 + 3 * 4 - 6 * 2 + 7 + 8 * 2"
                  "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
                  "+ 10 * (2 + 3 * (4 + 5)) ",
                  341);

int main()
{

    struct test_input input = {
        {0},
        TEST_INPUT_ONLY,
        /* TEST_INPUT_EXCLUDE, */
    };

    array_macro_append(&input.test_patterns, "*");

    tests_run_input(&input);
}
