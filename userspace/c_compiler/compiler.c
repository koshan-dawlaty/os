#include <container_of.h>
#include <fcntl.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/compiler/codegen_test123.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <unistd.h>

char *read_file_buffer(const char *pathname)
{
    int fd = open(pathname, O_RDONLY);

    size_t read_size = 10;
    size_t buf_len = read_size;

    char *buf = malloc(buf_len);
    size_t buf_offset = 0;
    size_t bytes_read;
    while ((bytes_read = read(fd, buf + buf_offset, read_size)) >= read_size)
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

    return buf;
}

void ir_test()
{

    const char *expr_str = MACRO_STR(int main() {
        /* return 5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9 + 2 + 3 * 4 - 6 * 2 + 7
         * + */
        /*        8 * 2 - 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8 + */
        /* return 10 * (2 + 3 * (4 + 5)); */
        /* return 2 * 3; */
        /* return (8 * 2) - (9 * 3); */
        /* int ptr = 40; */

        /* *ptr = 123; */

        /* return *ptr; */

        int i = 0;

        while (i <= 10)
        {
            i = i + 1;
        }

        return i;
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
    /* test123_generate_ir(&code, &program); */
    codegen_generate_ir(&code, &program);

    log_multiline_start(&LOG_DEBUG);
    logd("\n");
    for (size_t i = 0; i < code.num; i += 4)
    {
        /* logd("%p\n", *(u32 *)(code.items + i)); */
        /* printf("%p\n", *(u32 *)(code.items + i)); */
        /* logd("%p%p%p%p\n", code.items[i], code.items[i + 1], code.items[i +
         * 2], */
        /*      code.items[i + 3]); */
    }
    logd("\n");
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
    log_multiline_end(&LOG_DEBUG);

    run_code(code.items, code.num);
}

void run_from_str(const char *str)
{

    LIST_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(str, &tokens);

    LIST_DEFINE_HEAD(top_statements);
    struct ast_error *err = ast_parse(&tokens, &top_statements);

    struct ir_program program = {.register_offset = 0, 0, {0}};
    list_init_head(&program.instructions);
    struct ast_top_statement *main =
        containerof(top_statements.next, struct ast_top_statement, list);
    ir_generate_block(&program, &main->function->block);

    struct bytecode_array code = {0};
    codegen_generate_ir(&code, &program);
    printf("return:%d\n", run_code(code.items, code.num));
}

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        ir_test();
    }
    else
    {
        char *str = read_file_buffer(argv[1]);
        run_from_str(str);
        free(str);
    }
    return 0;
}
