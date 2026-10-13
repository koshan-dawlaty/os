#include <fcntl.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/interpreter.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{

    /* LIST_DEFINE_HEAD(expr_tokens); */
    /* tokenizer_parse_tokens("2 * (1 + 3) + (5)", &expr_tokens); */
    /* /1* tokenizer_parse_tokens("1 + (1 * 1)", &expr_tokens); *1/ */
    /* struct ast_parser parser = { */
    /*     .tokens = &expr_tokens, */
    /*     .current = containerof(expr_tokens.next, struct token, list)}; */
    /* struct ast_error *error = NULL; */
    /* struct ast_expr *expr = ast_parse_expression(&parser, &error); */
    /* logd("got expr"); */
    /* if (error != NULL) */
    /* { */
    /*     static char buf[100]; */
    /*     ast_error_to_string(error, buf, 100); */
    /*     PANIC("ast error:%s", buf); */
    /* } */

    /* struct interpreter *interp = interpreter_init(); */
    /* logd("evaluated:%d", expr_eval(expr, interp)->value); */

    /* exit(0); */

    LIST_DEFINE_HEAD(tokens);
    const char *program;
    if (argc <= 1)
    {
        // clang-format off
	program = MACRO_STR(

/* int fib(int num) { */
/* 	if (num > 2) { */
/* 		print("in function"); */
/* 		print(num); */
/* 		return fib(num - 1) + fib(num - 2); */
/* 	} */
/* 	return 1; */
/* } */


	

/* int main() { */
/* int x  = 10; */
/* int y  = 11; */
/* int* ptr = &x; */
/* 	print(x); */
/* 	print(y); */
/* 	print(ptr); */
/* 	*(ptr + 1) = 123; */
/* 	print(x); */
/* 	print(y); */
/* } */

int func(int num, int * result) {
	*result = num;
}
	
int main() {
	int result;
	func(1234, &result);
	print(result);
}


		);

        // clang-format on
    }
    else
    {
        int fd = open(argv[1], O_RDONLY);

        size_t read_size = 10;
        size_t buf_len = read_size;

        char *buf = malloc(buf_len);
        size_t buf_offset = 0;
        size_t bytes_read;
        while ((bytes_read = read(fd, buf + buf_offset, read_size)) >=
               read_size)
        {

            if (bytes_read < 0)
            {
                PANIC("bad fd");
            }

            buf_len += read_size;
            buf_offset += read_size;
            buf = realloc(buf, buf_len);
        }
        buf[buf_offset + bytes_read] = 0;

        program = buf;
    }
    tokenizer_parse_tokens(program, &tokens);

    /* ast_parse_tokens(&tokens); */

    LIST_DEFINE_HEAD(top_statements_head);

    struct ast_error *err = ast_parse(&tokens, &top_statements_head);
    if (err != NULL)
    {
        static char buf[100];
        ast_error_to_string(err, buf, 100);
        PANIC("ast error:%s", buf);
    }
    else
    {
        eval_program(&top_statements_head);
    }
    ast_top_statements_free(&top_statements_head);
    logd("done freeing");

    /* statements_eval_simple(&statements_head); */

    exit(0);

    return 0;
}
