#include "codegen.h"
#include "container_of.h"
#include <complex.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/c_compiler.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <tests/tests.h>
#include <unistd.h>
#include <utils.h>

extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);

struct bytecode_array
{
    u8 *items;
    size_t capacity;
    size_t num;

    /* offset from the stack pointer since entering the function */
    size_t stack_offset;

    /* list of struct variables in current scope */
    struct list current_scope_vars;
};

struct codegen_value
{
    enum codegen_value_type
    {
        CODEGEN_VALUE_REGISTER,
        CODEGEN_VALUE_MEMORY,
        CODEGEN_VALUE_IMMEDIATE,
        CODEGEN_VALUE_STACK,
    } type;
    union
    {
        u64 value;
        u8 reg;
    };
};

#define REG_IS_EXTENDED(reg) (reg & 0b1000)

struct codegen_value *codegen_value_alloc(enum codegen_value_type type,
                                          u64 value)
{
    struct codegen_value *out = malloc(sizeof(struct codegen_value));
    out->type = type;
    out->value = value;
    return out;
}

#define BYTE_AS_RM(byte) (byte & 0b111)

u8 get_modrm_bytes(u8 mod, u8 rm)
{
    return 0b11000000 | (mod & 0b111) << 3 | BYTE_AS_RM(rm);
}

u8 get_modrm_rm(u8 mod, struct codegen_value *rm)
{
    if (rm->type == CODEGEN_VALUE_STACK)
    {
        logd("stack rm");
        return 0b01000000 | BYTE_AS_RM(0b100) | ((mod & 0b111) << 3);
    }
    else
    {
        return 0b11000000 | (mod & 0b111) << 3 | BYTE_AS_RM(rm->reg);
    }
}

u8 get_modrm(struct codegen_value *reg, struct codegen_value *rm)
{
    if (rm->type == CODEGEN_VALUE_STACK)
    {
        logd("rm is memory, %d", reg->value);
        return 0b01000000 | BYTE_AS_RM(0b100) | ((reg->value) << 3);
    }
    else
    {
        return 0b11000000 | (reg->value & 0b111) << 3 | BYTE_AS_RM(rm->reg);
    }
}

/* a value for rex that will never be valid */
#define INVALID_REX 0

#define REX_PATTERN 0b01000000

static u8 get_rex(struct codegen_value *reg, struct codegen_value *rm)
{
    u8 out = REX_PATTERN;

    if (rm->type == CODEGEN_VALUE_REGISTER && REG_IS_EXTENDED(rm->reg))
    {
        out |= 0b1;
    }
    if (reg->type == CODEGEN_VALUE_REGISTER && REG_IS_EXTENDED(reg->reg))
    {
        out |= 0b100;
    }

    /* dont encode the rex pattern if it is not needed */
    return out == REX_PATTERN ? INVALID_REX : out;
}

void generate_opcode_bytes(struct bytecode_array *code, u8 rex, u8 opcode,
                           u8 mod)
{
    if (rex != INVALID_REX)
    {
        array_macro_append_v(code, rex);
    }
    array_macro_append_v(code, opcode, mod);
}

/* memory should be the rm value in the modrm byte */
void generate_sib_byte(struct bytecode_array *code,
                       struct codegen_value *memory)
{
    if (memory->type == CODEGEN_VALUE_STACK)
    {
        logd("generating sib byte, sp offset of %d",
             code->stack_offset - memory->value);
        array_macro_append_v(code, 0b00100100,
                             code->stack_offset - memory->value);
        /* memory->value - code->stack_offset); */
    }
}

struct codegen_value immediate_temp_reg = {CODEGEN_VALUE_REGISTER, 1};
struct codegen_value immediate_temp_reg2 = {CODEGEN_VALUE_REGISTER, 3};
struct codegen_value stack_stack_temp_reg = {CODEGEN_VALUE_REGISTER, 0b110};

void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src);

void generate_opcode(struct bytecode_array *code, u8 opcode,
                     struct codegen_value *dest, struct codegen_value *src)
{
    logd("%d, %d", dest->type, src->type);
    if (src->type == CODEGEN_VALUE_STACK && dest->type == CODEGEN_VALUE_STACK)
    {
        /* this might end up being annoying to debug because of recursion */
        generate_mov(code, &stack_stack_temp_reg, src);
        src = &stack_stack_temp_reg;
    }

    struct codegen_value *reg, *rm;
    if (dest->type == CODEGEN_VALUE_REGISTER)
    {
        logd("setting direction bit");
        opcode |= 0b10; /* r/m to register */
                        /* struct codegen_value *temp = dest; */
                        /* dest = src; */
                        /* src = temp; */
        rm = src;
        reg = dest;
    }
    else
    {
        logd("not setting direction bit");
        opcode &= ~0b10;
        rm = dest;
        reg = src;
    }

    logd("opcode:%p", opcode);

    logd("%d, %d", reg->type, rm->type);

    generate_opcode_bytes(code, get_rex(reg, rm), opcode, get_modrm(reg, rm));

    generate_sib_byte(code, rm);
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        array_macro_append_v(code, src->value, 0, 0, 0);
    }
}

void generate_opcode_modrm(struct bytecode_array *code, u8 opcode, u8 modrm,
                           struct codegen_value *dest,
                           struct codegen_value *src)
{
    generate_opcode_bytes(code, get_rex(dest, src), opcode, modrm);

    generate_sib_byte(code, dest);
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        array_macro_append_v(code, src->value, 0, 0, 0);
    }
}

void generate_opcode_mod(struct bytecode_array *code, u8 opcode, u8 mod,
                         struct codegen_value *dest, struct codegen_value *src)
{
    generate_opcode_bytes(
        code,
        get_rex(&(struct codegen_value){.type = CODEGEN_VALUE_IMMEDIATE, 0},
                dest),
        opcode, get_modrm_rm(mod, dest));

    generate_sib_byte(code, dest);
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        array_macro_append_v(code, src->value, 0, 0, 0);
    }
}

void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src)
{
    logd("mov:%d, %d,%d", dest->value, src->value, src->type);
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        generate_opcode_mod(code, 0xc7, 0, dest, src);
    }
    else
    {
        generate_opcode(code, 0x89, dest, src);
    }
}

void generate_ax_opcode(struct bytecode_array *code, u8 opcode, u8 mod,
                        struct codegen_value *src)
{
    generate_opcode_mod(code, opcode, mod, src,
                        &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0});
}

void generate_sub(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src)
{
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        logd("sub on:%d", dest->reg);
        generate_opcode_mod(code, 0x81, 0b101, dest, src);
    }
    else
    {
        generate_opcode(code, 0x29, dest, src);
    }
}

void generate_add(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src)
{
    /* if (src->type == CODEGEN_VALUE_STACK && dest->type ==
     * CODEGEN_VALUE_STACK) */
    /* { */
    /*     generate_mov(code, &immediate_temp_reg, src); */
    /*     src = &immediate_temp_reg; */
    /* } */
    if (src->type == CODEGEN_VALUE_IMMEDIATE)
    {
        generate_opcode_mod(code, 0x81, 0, dest, src);
    }
    else
    {
        generate_opcode(code, 3, dest, src);
    }
}

struct codegen_value *raise_sp(struct bytecode_array *code)
{
    code->stack_offset += 8;
    generate_sub(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
                 &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, 8});
    return codegen_value_alloc(CODEGEN_VALUE_STACK, code->stack_offset);
}
struct codegen_value *lower_sp(struct bytecode_array *code)
{
    code->stack_offset -= 8;
    generate_add(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
                 &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, 8});
    return codegen_value_alloc(CODEGEN_VALUE_STACK, code->stack_offset);
}

void generate_pop(struct bytecode_array *code, struct codegen_value *value)
{
    if (value->type == CODEGEN_VALUE_REGISTER)
    {
        if (REG_IS_EXTENDED(value->reg))
        {
            array_macro_append_v(code, 0b01000001);
        }

        code->stack_offset -= 8;
        array_macro_append_v(code, 0x58 + value->reg);
    }
    else
    {
        PANIC("not implemented");
    }
}

struct codegen_value *generate_push(struct bytecode_array *code,
                                    struct codegen_value *value)
{
    if (value->type == CODEGEN_VALUE_REGISTER)
    {
        if (REG_IS_EXTENDED(value->reg))
        {
            array_macro_append_v(code, 0b01000001);
        }

        code->stack_offset += 8;
        array_macro_append_v(code, 0x50 + value->reg);
        return codegen_value_alloc(CODEGEN_VALUE_STACK, code->stack_offset);
    }
    else
    {
        PANIC("not implemented");
    }
}

void generate_neg(struct bytecode_array *code, struct codegen_value *dest)
{
    ASSERT(dest->type == CODEGEN_VALUE_REGISTER, "cant negate not a register");
    /* TODO: maybe have a function that takes one argument for a value instead
     * of passing eax */
    generate_opcode_mod(
        code, 0xF7, 0b011, dest,
        &(struct codegen_value){.type = CODEGEN_VALUE_REGISTER, 0});
}

struct codegen_value *generate_mul(struct bytecode_array *code,
                                   struct codegen_value *one,
                                   struct codegen_value *two)
{

    struct codegen_value *out = codegen_value_alloc(CODEGEN_VALUE_REGISTER, 0);
    generate_mov(code, out, two);
    generate_mov(code, &immediate_temp_reg, one);
    generate_ax_opcode(code, 0xF7, 0b100, &immediate_temp_reg);
    /* ax */
    return out;
}

/* duplicated in interpreter */
struct variable
{
    char *name;
    struct codegen_value *value;
    struct list list;
};

struct variable *variable_alloc(const char *name, struct codegen_value *value)
{
    struct variable *out = malloc(sizeof(struct variable));
    out->name = strdup(name);
    out->value = value;
    out->list = LIST_INIT();
    return out;
}

void variable_free(struct variable *var)
{
    free(var->name);
    free(var->value);
    free(var);
}

struct variable *variable_lookup(struct bytecode_array *code, const char *name)
{
    struct list *pos;
    list_for_each(pos, &code->current_scope_vars)
    {
        struct variable *var = containerof(pos, struct variable, list);
        if (!strcmp(var->name, name))
        {
            return var;
        }
    }

    return NULL;
}

struct codegen_value *generate_operator(struct bytecode_array *code,
                                        struct codegen_value *lhs,
                                        struct codegen_value *rhs,
                                        enum token_type operator_type)
{

    struct codegen_value *retval;
    switch (operator_type)
    {
    case TOKEN_PLUS:;
        struct codegen_value *stack = raise_sp(code);
        generate_mov(code, stack,
                     codegen_value_alloc(CODEGEN_VALUE_IMMEDIATE, 0));
        if (lhs != NULL)
        {
            logd("adding:%d", lhs->value);
            generate_add(code, stack, lhs);
        }

        if (rhs != NULL)
        {
            logd("adding:%d", rhs->value);
            generate_add(code, stack, rhs);
        }

        retval = stack;
        break;
    case TOKEN_MINUS:;
        struct codegen_value *temp_reg = &immediate_temp_reg;

        if (lhs->type == CODEGEN_VALUE_IMMEDIATE &&
            rhs->type == CODEGEN_VALUE_IMMEDIATE)
        {
            generate_mov(code, temp_reg, lhs);
            generate_sub(code, temp_reg, rhs);
            retval = generate_push(code, temp_reg);
        }
        else if (lhs->type == CODEGEN_VALUE_IMMEDIATE)
        {
            generate_mov(code, temp_reg, lhs);
            generate_mov(code, &immediate_temp_reg2, rhs);
            generate_neg(code, &immediate_temp_reg2);
            generate_add(code, &immediate_temp_reg, &immediate_temp_reg2);
            retval = generate_push(code, &immediate_temp_reg);
        }
        else
        {
            generate_mov(code, temp_reg, lhs);
            generate_sub(code, temp_reg, rhs);
            retval = generate_push(code, temp_reg);
        }
        break;
    case TOKEN_STAR:;
        struct codegen_value *value = generate_mul(code, lhs, rhs);
        retval = generate_push(code, value);
        break;

    case TOKEN_ASSIGN:
        generate_mov(code, lhs, rhs);
        return lhs;

    default:
        PANIC("invalid operator type from ast:%s",
              token_type_str(operator_type));
    }
    /* if (lhs->type == CODEGEN_VALUE_STACK) */
    /* { */
    /*     lower_sp(code); */
    /* } */
    /* if (rhs->type == CODEGEN_VALUE_STACK) */
    /* { */
    /*     lower_sp(code); */
    /* } */
    return retval;
}

struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr);

struct codegen_value *generate_expr_impl(struct bytecode_array *code,
                                         struct ast_expr *expr)
{

    switch (expr->type)
    {
    case AST_EXPRESSION_VALUE:
        if (expr->value.is_var)
        {
            struct variable *var = variable_lookup(code, expr->value.varname);
            NOT_NULL(var, "variable not found %s", expr->value.varname);
            return var->value;
        }
        else
        {
            logd("immediate %d", expr->value.literal.integer);
            return codegen_value_alloc(CODEGEN_VALUE_IMMEDIATE,
                                       expr->value.literal.integer);
        }
        break;
    case AST_EXPRESSION_CALL:
        break;
    case AST_EXPRESSION_OPERATOR:;

        struct codegen_value *lhs = generate_expr_impl(code, expr->lhs);
        struct codegen_value *rhs = generate_expr_impl(code, expr->rhs);

        /* if (lhs != NULL && rhs != NULL) */
        /* { */
        /*
         * FIXME: this is kind of weird, it might be a good idea to split
         * this into multiple functions (similar to parsing expressions for
         * the ast)
         */
        return generate_operator(code, lhs, rhs, expr->operator);
        /* } */

        break;
    case AST_EXPRESSION_UNARY_OPERATOR:
    default:
        PANIC("unknown expression type");
        break;
    }
}

struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr)
{

    size_t stack_start = code->stack_offset;
    struct codegen_value *result = generate_expr_impl(code, expr);

    /* reset stack offset */
    generate_add(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
                 &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE,
                                         code->stack_offset - stack_start});
    code->stack_offset = stack_start;

    return result;
}

void generate_return(struct bytecode_array *code)
{
    struct list *pos;
    list_for_each(pos, &code->current_scope_vars)
    {
        logd("lowering sp");
        lower_sp(code);
    }
    array_macro_append_v(code, 0xc3);
}

void generate_statement(struct bytecode_array *code,
                        struct ast_statement *statement)
{
    switch (statement->type)
    {
    case AST_STATEMENT_EXPR:
        generate_expr(code, statement->expr);
        break;
    case AST_STATEMENT_IF:
    case AST_STATEMENT_WHILE:
    case AST_STATEMENT_FOR:
        break;
    case AST_STATEMENT_DECLARE:;
        struct variable *var =
            variable_alloc(statement->varname, raise_sp(code));

        if (statement->init_value != NULL)
        {
            struct codegen_value *init_value =
                generate_expr(code, statement->init_value);
            generate_mov(code, var->value, init_value);
        }
        list_push(&var->list, &code->current_scope_vars);

        break;
    case AST_STATEMENT_RETURN:;
        struct codegen_value *result = generate_expr(code, statement->expr);
        generate_mov(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0},
                     result);

        generate_return(code);
        break;
    }
}

void generate_function(struct bytecode_array *code,
                       struct ast_top_statement *function)
{
    struct list *pos;
    list_for_each(pos, &function->function->block)
    {
        struct ast_statement *s = containerof(pos, struct ast_statement, list);
        generate_statement(code, s);
    }

    /* generate_return(code); */
}

int run_code(u8 *bytecode, size_t len)
{

    int retval = ((int (*)())bytecode)();

    logd("returned:%d", retval);

    return retval;

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

int generate_and_run_expr(struct ast_expr *expr)
{

    struct bytecode_array code = {0};

    struct codegen_value *result = generate_expr(&code, expr);
    generate_mov(&code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0},
                 result);

    /* reset stack offset */
    generate_add(
        &code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
        &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, code.stack_offset});

    array_macro_append_v(&code, 0xc3);

    int retval = run_code(code.items, code.num);

    free(code.items);

    return retval;
}

extern void (*__ctors_start[])(void);
extern void (*__ctors_end[])(void);
void call_constructors(void)
{
    for (void (**ctor)(void) = __ctors_start; ctor < __ctors_end; ++ctor)
    {
        (*ctor)();
    }
}

int main()
{
    call_constructors();

    LIST_DEFINE_HEAD(tokens);
    const char *program = MACRO_STR(
        // clang-format off

	int main() {
		int x = 2 * 3;
		int y = x + 1;
		int z = x * y;
		return z;
	}

        // clang-format on
    );

#ifdef TESTS
    run_tests();
#endif

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
