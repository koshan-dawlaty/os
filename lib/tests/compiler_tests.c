#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/list.h>
#include <lib/log.h>
#include <tests/tests.h>

struct test_result *test_codegen_expr(const char *program, int expected)
{
    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(program, &tokens);

    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_error *err;
    struct ast_expr *expr = ast_parse_expression(&parser, &err);
    LIST_DEFINE_HEAD(top_statements_head);
    struct bytecode_array code = {0};
    list_init_head(&code.current_scope_vars);
    generate_expr(&code, expr);

    int got = run_code(code.items, code.num);
    ASSERT_EQ(got, expected, ", %s (%d) != %d", program, expected, got);

    TEST_OK();
}

#define TEST_CODEGEN_EXPR(name, expr, expected)                                \
    TEST(name, { return test_codegen_expr(expr, expected); })

TEST_CODEGEN_EXPR(test_codegen_expr1, "1+1", 2);
TEST_CODEGEN_EXPR(test_codegen_expr2, "2 + 3 * 4 - 6 * 2 + 5", 7);
TEST_CODEGEN_EXPR(test_codegen_expr3,
                  "  5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
                  "+ 2 + 3 * 4 - 6 * 2 + 5 + 7 + 8 * 2"
                  "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
                  "+ 10 * (2 + 3 * (4 + 5)) ",
                  346);
