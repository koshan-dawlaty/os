/* TODO: this has to be in userspace for right now, maybe move to kernel tests?
 */

/* this is a test executable, so always run tests */
#ifndef TESTS
#define TESTS
#include "lib/utils.h"
#endif

#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/list.h>
#include <lib/log.h>
#include <tests/tests.h>

struct test_result *test_codegen_program(const char *program_str, int expected)
{

    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(program_str, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);

    struct ir_program program = {.register_offset = 0, 0, {0}};
    list_init_head(&program.instructions);
    struct ast_top_statement *main =
        containerof(top_statements.next, struct ast_top_statement, list);
    ir_generate_block(&program, &main->function->block);

    struct bytecode_array code = {0};
    codegen_generate_ir(&code, &program);
    int got = run_code(code.items, code.num);
    ASSERT_EQ(got, expected, ", %s (%d) != %d", program_str, expected, got);

    TEST_OK();
}

#define TEST_CODEGEN_EXPR(name, expr, expected)                                \
    TEST(name, {                                                               \
        return test_codegen_program("int main() { return" expr ";}",           \
                                    expected);                                 \
    })

TEST_CODEGEN_EXPR(test_codegen_expr1, "1 + 1", 2);
TEST_CODEGEN_EXPR(test_codegen_expr2, "2 + 3 * 4 - 6 * 2 + 5", 7);
TEST_CODEGEN_EXPR(test_codegen_expr3,
                  "  5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
                  "+ 2 + 3 * 4 - 6 * 2 + 7 + 8 * 2"
                  "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
                  "+ 10 * (2 + 3 * (4 + 5)) ",
                  341);

#define TEST_CODEGEN(name, program, expected)                                  \
    TEST(name, { return test_codegen_program(program, expected); })

// clang-format off
TEST_CODEGEN(codegen_test_vars, MACRO_STR(int main() {
                 int x = 10;
                 int y = 20;
                 int z = y + x;
                 return z;
             }),
             30)

TEST_CODEGEN(codegen_test_vars_assign, MACRO_STR(int main() {
                 int x = 10;
                 int y = 20;
                 y = x;
                 return y;
             }),
             10)

TEST_CODEGEN(codegen_test_vars_assign2, MACRO_STR(int main() {
                 int x = 10;
                 int y = 20;
                 int z = 30;
                 int w = x + y + z;
                 z = y;
                 int f = w + z;
                 return f;
             }),
             80)

TEST_CODEGEN(codegen_test_if_basic_true, MACRO_STR(int main() {
                 int x = 10;
                 if (x)
                 {
                     return 1;
                 }
                 return 2;
             }),
             1)

TEST_CODEGEN(codegen_test_if_basic_false, MACRO_STR(int main() {
			 int x = 0;
			 if (x)
			 {
				 return 1;
			 }
			 return 2;
             }),
             2)

TEST_CODEGEN(codegen_test_if_equals_true, MACRO_STR(
	int main()
	{
		 int x = 10;
		 if (x == 10)
		 {
			 return 1;
		 }
		 return 2;
	 }
),1)

TEST_CODEGEN(codegen_test_if_equals_false, MACRO_STR(
	int main()
	{
		 int x = 5;
		 if (x == 10)
		 {
			 return 1;
		 }
		 return 2;
	 }
),2)

TEST_CODEGEN(codegen_test_if_and_true, MACRO_STR(
	int main()
	{
		 int x = 5;
		 int y = 10;
		 if (x == 5 && y == 10)
		 {
			 return 1;
		 }
		 return 2;
	 }
),1)

TEST_CODEGEN(codegen_test_if_and_false1, MACRO_STR(
	int main()
	{
		 int x = 3;
		 int y = 10;
		 if (x == 5 && y == 10)
		 {
			 return 1;
		 }
		 return 2;
	 }
),2)

TEST_CODEGEN(codegen_test_if_and_false2, MACRO_STR(
	int main()
	{
		 int x = 5;
		 int y = 9;
		 if (x == 5 && y == 10)
		 {
			 return 1;
		 }
		 return 2;
	 }
),2)

/* all of the phi tests don't actually work when generating ssa,
 * ssa does not work at time of writing */
TEST_CODEGEN(codegen_test_phi_basic1, MACRO_STR(
	int main()
	{
        int y = 0;
        int x = 2;
        if (y)
        {
            x = 1;
        }
        return x;
	 }
),2)

TEST_CODEGEN(codegen_test_phi_basic2, MACRO_STR(
	int main()
	{
        int y = 1;
        int x = 2;
        if (y)
        {
            x = 1;
        }
        return x;
	 }
),1)


TEST_CODEGEN(codegen_test_while, MACRO_STR(
	int main()
	{
		int i = 0;
		while (i <= 10) {
			i = i + 1;
		}

		return i;
	 }
),11)

TEST_CODEGEN(codegen_test_nested_if, MACRO_STR(
	int main() {
		int i = 0;
		if (i <= 6) {
			if (i <= 5) {
				if (i <= 4) {
					if (i <= 3) {
						if (i <= 2) {
							if (i <= 1) {
								return 123;
							}
						} 
					} 
				} 
			}
		}
		return 456;
	}
	), 123)

TEST_CODEGEN(codegen_test_for_basic1, MACRO_STR(int main() {
        int i = 0;
        for (i = 0; i <= 10; i = i + 1)
        {
        }

        return i;
    }
	), 11)

TEST_CODEGEN(codegen_test_for_basic2, MACRO_STR(int main() {
        int i = 0;
        int j = 0;
        for (i = 0; i <= 10; i = i + 1)
        {
            j = j + 3;
        }

        return j;
    }
	), 33)

TEST_CODEGEN(codegen_test_stack_addr, MACRO_STR(int main() {
        int x = 123;
        int *ptr = &x;
        int value = *ptr;

        return value;
    }
	), 123)

// clang-format on
//
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
