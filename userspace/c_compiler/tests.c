#include "c_compiler.h"
#include "lib/utils.h"
#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/list.h>
#include <lib/log.h>
#include <math.h>
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
    LIST_DEFINE_HEAD(token_list);
    tokenizer_parse_tokens(program, &token_list);

    size_t i = 0;
    struct list *pos;
    list_for_each(pos, &token_list)
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
    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(expr_str, &tokens);
    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_error *err;
    struct ast_expr *expr = ast_parse_expression(&parser, &err);
    if (expr == NULL)
    {
        static char buf[100];
        ast_error_to_string(err, buf, 100);
        FAIL_TEST("ast expression parsing failed with error %s", buf);
    }
    struct interpreter *interpreter = interpreter_init();
    struct interp_value *interp_val = expr_eval(expr, interpreter);
    int val = interp_val->value;
    ASSERT_EQ(val, expected, "%d != %d for expr %s", val, expected, expr_str);

    TEST_OK();
}

#define TEST_AST_EXPRESSIONS(name, expr, expected)                             \
    TEST(name, { return test_ast_expressions(expr, expected); })

TEST_AST_EXPRESSIONS(expr_test1, "1 + 1", 2)
TEST_AST_EXPRESSIONS(expr_test2, "1 + ((2 + 3) + 4) ", 10)
TEST_AST_EXPRESSIONS(expr_test3, "1+1+1+1+1+1", 6)
TEST_AST_EXPRESSIONS(expr_test4, "(((1+2)+2)+2) + 3", 10)
TEST_AST_EXPRESSIONS(expr_test5, "(1+2) + 3", 6)
TEST_AST_EXPRESSIONS(expr_test6, "123", 123)

TEST_AST_EXPRESSIONS(expr_test_less_equal_true1, "1 <= 1", true)
TEST_AST_EXPRESSIONS(expr_test_less_equal_true2, "1 <= 3", true)
TEST_AST_EXPRESSIONS(expr_test_less_equal_false1, "1 <= 0", false)
TEST_AST_EXPRESSIONS(expr_test_less_equal_false2, "3 <= 1", false)
TEST_AST_EXPRESSIONS(expr_test_greater_equal_false1, "1 >= 2", false)
TEST_AST_EXPRESSIONS(expr_test_greater_equal_false2, "1 >= 3", false)
TEST_AST_EXPRESSIONS(expr_test_greater_equal_true1, "1 >= 1", true)
TEST_AST_EXPRESSIONS(expr_test_greater_equal_true2, "3 >= 1", true)
TEST_AST_EXPRESSIONS(expr_test_not_equal_true, "1 != 2", true)
TEST_AST_EXPRESSIONS(expr_test_not_equal_false, "1 != 1", false)

TEST_AST_EXPRESSIONS(expr_test_order_of_operations1, "2 * 3 + 4", 10)
TEST_AST_EXPRESSIONS(expr_test_order_of_operations2, "4 + 2 * 3", 10)
TEST_AST_EXPRESSIONS(expr_test_order_of_operations3, "4 + 2 * 3 == 10", 1)
TEST_AST_EXPRESSIONS(expr_test_order_of_operations4, "(2 + 3) * 4", 20)

TEST_AST_EXPRESSIONS(
    expr_test_order_of_operations_long,
    "7 + 3 * 5 - 2 * 8 + 9 * 4 - 6 + 11 * 3 - 5 * 7 + 8 * 2 - 13 + 4 * 9 - 10 "
    "* 6 + 12 * 5 - 7 + 14 * 3 - 9 * 8 + 15 - 2 * 11 + 6 * 7 - 4 + 13 * 2 - 5 "
    "* 9 + 8 * 10 - 12 + 3 * 14 - 6 * 5 + 17 - 4 * 8 + 9 * 11 - 7 + 10 * 12 - "
    "13 * 2 + 5 * 6 - 14 + 8 * 9",
    387)

TEST_AST_EXPRESSIONS(expr_test_order_of_operations_parentheses,
                     " 3 + 5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
                     "+ 2 + 3 * 4 - 6 * 2 + 5 + 7 + 8 * 2"
                     "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
                     "+ 10 * (2 + 3 * (4 + 5)) - 7 * 8 + 9"
                     "- 11 + 2 * 3 * (4 + 5) - 6 * (7 + 8)",
                     255)

struct test_result *test_interpreter_error(const char *program,
                                           enum ast_error_type expected)
{
    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);
    if (err == NULL)
    {
        /* FIXME: maybe have a function to also convert a ast_error_type to a
         * string */
        FAIL_TEST("expected error %d, but did not get an error", expected);
    }

    ASSERT_EQ(expected, err->type,
              "expected error of type %d, but got error of type %d", expected,
              err->type);

    TEST_OK();
}

struct test_result *test_interpreter(const char *program, int expected)
{
    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);
    if (err != NULL)
    {
        static char buf[100];
        ast_error_to_string(err, buf, 100);
        FAIL_TEST("ast parsing failed with error %s", buf);
    }

    int got = eval_program(&top_statements);
    ASSERT_EQ(got, expected, "%d != %d for program %s", got, expected, program);
    ast_top_statements_free(&top_statements);

    TEST_OK();
}

#define TEST_INTERPRETER(name, program_expr, expected)                         \
    TEST(name, { return test_interpreter(MACRO_STR(program_expr), expected); })
#define TEST_INTERPRETER_ERROR(name, program_expr, expected)                   \
    TEST(name, { return test_interpreter_error(program_expr, expected); })

// clang-format off
TEST_INTERPRETER(interpreter_test_add_vars,
int main() {
	int x = 10;
	int y = x +  5;
	return  y;
}
, 15);

TEST_INTERPRETER(interpreter_test_multiply,
int main() {
int x = 10;
int y = 2;

return y * x;
}
, 20);


TEST_INTERPRETER(interpreter_test_if_equals,
int main() {
	int x = 10;
	int y = 10;
	y= y + 5;
	y = y - 5;
	if (x == y) {
	return 123;
	}

	return 1;
}
, 123);


TEST_INTERPRETER(interpreter_test_basic_return,
int main() {
	return  123;
}
, 123);

TEST_INTERPRETER(interpreter_test_call_arg,

int func(int num) {
	return num + 5;
}

int main() {
	return func(20);
}
, 25);

TEST_INTERPRETER(interpreter_test_declare_assign_function_retval, 

int func(int num) {
	return num + 3;
}

int main() {
	int retval = func(120);
	return retval;
}

, 123)

TEST_INTERPRETER(interpreter_test_call_before_first_scope_declare, 

int func(int num) {
	return num + 5;
}

int main() {
	func(50);
	int retval = 123;
	return retval;
}

, 123)

TEST_INTERPRETER(interpreter_test_call_passing_arg_to_other_arg,
int f2(int num2) {
	return num2 + 10;
}
int f1(int num) {
	return f2(num + 5);
}
int main() {
	return f1(15);
}
, 30);

TEST_INTERPRETER(interpreter_test_if_assign,
int main() {
	int x = 10;
	if (x > 5) {
		x = x + 5;
	}
	return x;
}
, 15);

TEST_INTERPRETER(interpreter_test_if_return,
int main() {
	int x = 10;
	if (x > 5) {
		return 123;
	}
	return 456;
}
, 123);

TEST_INTERPRETER(interpreter_test_loop_while,
int main() {
	int i = 0;
	while (i < 10) {
		i = i + 1;
	}
	return i;
}
, 10);

TEST_INTERPRETER(interpreter_test_derefrence_literal_string,
int main() {
	char * str = "abc";

	return *str;
}
, 97);

TEST_INTERPRETER(interpreter_test_strlen,

int strlen(char * str) {
	int i = 0;
	while (*(str + i)) {
		i = i + 1;
	}
	return i;
}
	
int main() {
	char * s = "abc 123";
	return strlen(s);
}
, 7);

TEST_INTERPRETER(interpreter_test_derefrence_offset_str,
int main() {
	char * str = "abc";
	

	return *(str + 1);
}
, 98);


TEST_INTERPRETER(interpreter_test_derefrence_as_lvalue,
int main() {
	char * str = "abc";
	*str = 123;

	return *str;
}
, 123);

TEST_INTERPRETER(interpreter_test_derefrence_as_lvalue_offset,
int main() {
	char * str = "abc";
	*(str + 3) = 123;

	return *(str + 3);
}
, 123);

TEST_INTERPRETER(interpreter_test_derefrence_no_use,
int main() {
	char * str = "abc";
	*str;

	return 123;
}
, 123);

TEST_INTERPRETER(interpreter_test_loop_while_early_return,
int main() {
	int i = 0;
	while (i < 10) {
		if (i > 5) {
			return 123;
		}
		i = i + 1;
	}
	return i;
}
, 123);


TEST_INTERPRETER(interpreter_test_char_literal_basic,
	int main() {
	return  'a';
	}
		, 97) 

TEST_INTERPRETER(interpreter_test_nested_if,
int main() {
	int i = 10;
	if (i > 1) {
		if (i > 2) {
			if (i > 3) {
				if (i > 4) {
					if (i > 5) {
						if (i > 6) {
							if (i > 7) {
								return 123;
							}
						}
					} 
				} 
			} 
		}
	}
	return 456;
 
}
, 123);

TEST_INTERPRETER(interpreter_test_for_return_in_loop,
	int main() {
		int i;
		for (i = 0; i < 100; i = i + 1) {
			if (i == 10) {
			return 123;
			}	
		}
		return 456;
	}
		, 123) 

TEST_INTERPRETER(interpreter_test_for_assign, 
	int main() {
		int i = 456;
		for (i = 123; 1 == 0; i = i + 1) {
			print("execution should not get here");
		}
		return i;
	}
	,	123);

TEST_INTERPRETER(interpreter_test_for_sum, 
	int main() {
		int sum;
		int i;
		for (i = 1; i < 11 ; i=  i +  1) {
		sum =sum + i;
		}
		return sum;
	}
	,	55);

TEST_INTERPRETER(interpreter_test_pow,
int pow(int num1, int num2) {
int i = 0;
int sum = 1 ;
while (i < num2) {
sum = sum * num1;	
i = i + 1;
}
return sum;
}

int main() {
	return pow(2, 5);
}

, 32);

TEST_INTERPRETER(interpreter_test_fibonacci,
int fib(int num) {
	if (num > 2) {
		return fib(num - 1) + fib(num - 2);
	}
	return 1;
}

int main() {
	return fib(10);
}

, 55);


TEST_INTERPRETER_ERROR(interpreter_test_semicolon_expected_expression,
	
	"int main() {"
		"int x;"
		"x = x + 1"
	"}"	

, AST_ERROR_SEMICOLON_EXPECTED)

TEST_INTERPRETER_ERROR(interpreter_test_semicolon_expected_declaration,
	
	"int main() {"
		"int x = 123"
		"x = x + 1;"
	"}"	

, AST_ERROR_SEMICOLON_EXPECTED)


TEST_INTERPRETER_ERROR(interpreter_test_argument_expected,
	
	"int main(int x ; ) {"
	"}"	

, AST_ERROR_ARUGMENT_EXPECTED)


TEST_INTERPRETER_ERROR(interpreter_test_identifier_expected_arg,
	
	"int main(int) {"
	"}"	

, AST_ERROR_IDENTIFIER_EXPECTED)

// clang-format on
