#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/c_compiler.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <utils.h>

extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);

#define PREFIX_64 (0x48)

struct bytecode_array
{
    u8 *items;
    size_t capacity;
    size_t num;
};

struct codegen_value
{
    enum codegen_value_type
    {
        CODEGEN_VALUE_REGISTER,
        CODEGEN_VALUE_MEMORY,
        CODEGEN_VALUE_IMMEDIATE,
    } type;
    union
    {
        u64 value;
        u8 reg;
    };
};
struct codegen_value *codegen_value_alloc(enum codegen_value_type type,
                                          u64 value)
{
    struct codegen_value *out = malloc(sizeof(struct codegen_value));
    out->type = type;
    out->value = value;
    return out;
}

u8 get_modrm(struct codegen_value *reg, struct codegen_value *rm)
{
    u8 out = 0b11000000;

    out |= (reg->reg & 0b111);
    /* if (rm->type == CODEGEN_VALUE_REGISTER) */
    /* { */
    /*     out |= (rm->reg & 0b111) << 3; */
    /* } */

    /* logd("got this mod/rm:%p", out); */

    return out;
}

/* FIXME: the name should represent that this does not work for every opcode */
void generate_opcode(struct bytecode_array *code, u8 opcode,
                     struct codegen_value *dest, struct codegen_value *src)
{
    /* FIXME: this should use prefixes */
    array_macro_append_v(code, opcode, get_modrm(dest, src));
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        logd("appending:%d", src->value);
        array_macro_append_v(code, src->value, 0, 0, 0);
    }
}

void generate_add(struct bytecode_array *code, struct codegen_value *src,
                  struct codegen_value *dest)
{
    logd("src reg:%d", src->reg);
    if (dest->type == CODEGEN_VALUE_IMMEDIATE)
    {
        generate_opcode(code, 0x81, src, dest);
    }
    else
    {
        generate_opcode(code, 1, src, dest);
    }
}

struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr,
                                    struct codegen_value *dest)
{

    switch (expr->type)
    {
    case AST_EXPRESSION_VALUE:
        if (expr->value.is_var)
        {
            return codegen_value_alloc(CODEGEN_VALUE_REGISTER, 0);
        }
        else
        {
            return codegen_value_alloc(CODEGEN_VALUE_IMMEDIATE,
                                       expr->value.literal.integer);
        }
    case AST_EXPRESSION_CALL:
        break;
    case AST_EXPRESSION_OPERATOR:;

        struct codegen_value *lhs = generate_expr(code, expr->lhs, dest);
        struct codegen_value *rhs = generate_expr(code, expr->rhs, dest);

        if (lhs != NULL)
        {
            generate_add(code, dest, lhs);
        }

        if (rhs != NULL)
        {
            generate_add(code, dest, rhs);
        }

        return NULL;
        break;
    case AST_EXPRESSION_UNARY_OPERATOR:
        PANIC("");
        break;
    }
}

void run_code(u8 *bytecode, size_t len)
{

    int retval = ((int (*)())bytecode)();
    logd("%d", retval);

    return;

    u8 *binary_test = malloc(9 + sizeof(bytecode));
    binary_test[0] = 'T';
    binary_test[1] = 'E';
    binary_test[2] = 'S';
    binary_test[3] = 'T';
    binary_test[4] = 0;
    *(u32 *)(binary_test + 5) = sizeof(bytecode);
    memcpy(binary_test + 9, bytecode, len);
    syscall_wrapper_asm(SYS_load_simple_binary, (ptr_t)binary_test, 0, 0, 0, 0);
}

int main()
{
    LIST_DEFINE_HEAD(tokens);
    const char *program = MACRO_STR(
        // clang-format off
		
		1 + 2  + 3

        // clang-format on
    );
    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements_head);

    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_error *err;
    struct ast_expr *expr = ast_parse_expression(&parser, &err);

    struct bytecode_array code = {0};
    struct codegen_value dest = {CODEGEN_VALUE_REGISTER, 0};
    generate_expr(&code, expr, &dest);

    array_macro_append_v(&code, 0xc3);

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

    exit(0);
}
