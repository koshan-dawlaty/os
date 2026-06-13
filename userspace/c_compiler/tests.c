#include "c_compiler.h"
#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <tests/tests.h>

struct tokenizer_test_input
{
    const char *str;
    enum token_type type;
};

struct test_result *tokenizer_test_impl(const char *program,
                                        struct tokenizer_test_input expected[],
                                        size_t expected_len)
{
    LIST2C_DEFINE_HEAD(token_list);
    tokenizer_parse_tokens(program, &token_list);

    size_t i = 0;
    struct list2c *pos;
    list2c_for_each(pos, &token_list)
    {
        struct token *tok = containerof(pos, struct token, list);

        if (i >= expected_len)
        {

            FAIL_TEST("%d more tokens than expected %d in output", i,
                      expected_len - 1);
        }

        if (tok->type == TOKEN_UNKNOWN)
        {
            FAIL_TEST("unknown token:%s", tok->token);
        }

        if (expected[i].type != tok->type)
        {
            FAIL_TEST("expected token of type:%s, got %s",
                      token_type_str(expected[i].type),
                      token_type_str(tok->type));
        }

        if (strcmp(expected[i].str, tok->token))
        {
            FAIL_TEST("expected token of value:|%s|, got |%s|", expected[i].str,
                      tok->token);
        }

        i++;
    }

    if (i != expected_len)
    {
        FAIL_TEST("less tokens than expected in output");
    }
    TEST_OK();
}

#define tokenizer_test(name, program, ...)                                     \
    TEST(name, {                                                               \
        return tokenizer_test_impl(                                            \
            program, ((struct tokenizer_test_input[]){__VA_ARGS__}),           \
            sizeof(((struct tokenizer_test_input[]){__VA_ARGS__})) /           \
                sizeof(struct tokenizer_test_input));                          \
    })

// clang-format off
tokenizer_test(tokenizer_test1, "int   x   =   10   ;    ",
			   {"int", TOKEN_IDENTIFIER},
			   {"x", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"10", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},
			   {"", TOKEN_EOF},
			   );


tokenizer_test(tokenizer_test2, "int test123=10; int x = 123;",
			   {"int", TOKEN_IDENTIFIER},
			   {"test123", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"10", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},

			   {"int", TOKEN_IDENTIFIER},
			   {"x", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"123", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},
			   {"", TOKEN_EOF},
			   );

tokenizer_test(tokenizer_test3, 
		"void main(int argc) {"
		"	print(123);"
		"}",

			   {"void", TOKEN_IDENTIFIER},
			   {"main", TOKEN_IDENTIFIER},
			   {"(", TOKEN_O_PAREN},
			   {"int", TOKEN_IDENTIFIER},
			   {"argc", TOKEN_IDENTIFIER},
			   {")", TOKEN_C_PAREN},
			   {"{", TOKEN_O_BRACE},
			   {"print", TOKEN_IDENTIFIER},
			   {"(", TOKEN_O_PAREN},
			   {"123", TOKEN_INT},
			   {")", TOKEN_C_PAREN},
			   {";", TOKEN_SEMICOLON},
			   {"}", TOKEN_C_BRACE},
			   {"", TOKEN_EOF},

			   );
// clang-format on

struct test_result *test_ast_expressions(char *expr_str, int expected)
{
    LIST2C_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(expr_str, &tokens);
    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_expr *expr = ast_parse_expression(&parser);
    struct interpreter *interpreter = interpreter_init();
    int val = expr_eval(expr, interpreter);
    ASSERT_EQ(val, expected, "%d != %d for expr %s", val, expected, expr_str);

    TEST_OK();
}

#define TEST_AST_EXPRESSIONS(name, expr, expected)                             \
    TEST(name, { return test_ast_expressions(expr, expected); })

TEST_AST_EXPRESSIONS(expr_test1, "1 + ((2 + 3) + 4) ", 10)
TEST_AST_EXPRESSIONS(expr_test2, "1+1+1+1+1+1", 6)
TEST_AST_EXPRESSIONS(expr_test3, "(((1+2)+2)+2) + 3", 10)
TEST_AST_EXPRESSIONS(expr_test4, "(1+2) + 3", 6)
TEST_AST_EXPRESSIONS(expr_test5, "123", 123)
