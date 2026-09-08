#include <complex.h>
#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/codegen.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <tests/tests.h>
#include <unistd.h>
#include <utils.h>

extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);

/* true if the type can be used in the r/m field in the modrm byte */
#define CODEGEN_TYPE_IS_RM(type)                                               \
    (type == CODEGEN_VALUE_REGISTER || type == CODEGEN_VALUE_STACK ||          \
     type == CODEGEN_VALUE_MEMORY)

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
    if (dest->type == CODEGEN_VALUE_REGISTER &&
        src->type != CODEGEN_VALUE_REGISTER)
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

/* opcode argument is what goes after the 0x0F opcode prefix */
void generate_set_conditional(struct bytecode_array *code, u8 opcode,
                              struct codegen_value *dest)
{
    ASSERT(CODEGEN_TYPE_IS_RM(dest->type),
           "set conditional can only operate on r/m");
    /* TODO: this should use generate_opcode_mod(), but it has a 2 byte opcode,
     * and generate_opcode_mod() does not support that, the opcode probably
     * can't be appended to the array before this, because the rex prefix could
     * exist for @dest */
    ASSERT(
        !(dest->type == CODEGEN_VALUE_REGISTER && REG_IS_EXTENDED(dest->reg)),
        "2 byte opcodes not really implemented");

    array_macro_append_v(code, 0x0F);

    generate_opcode_mod(code, opcode, 0, dest,
                        &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0});
    /* array_macro_append_v(code, 0x0F, opcode, 0xC0 + dest->reg); */
}

void generate_setz(struct bytecode_array *code, struct codegen_value *dest)
{
    generate_set_conditional(code, 0x94, dest);
}
void generate_setnz(struct bytecode_array *code, struct codegen_value *dest)
{
    generate_set_conditional(code, 0x95, dest);
}

void generate_cmp(struct bytecode_array *code, struct codegen_value *reg1,
                  struct codegen_value *reg2)
{
    if (reg1->type == CODEGEN_VALUE_IMMEDIATE)
    {
        generate_opcode_mod(code, 0x81, 0b111, reg2, reg1);
    }
    else if (reg2->type == CODEGEN_VALUE_IMMEDIATE)
    {
        generate_opcode_mod(code, 0x81, 0b111, reg1, reg2);
    }
    else
    {
        generate_opcode(code, 0x3B, reg1, reg2);
    }
}
void generate_test(struct bytecode_array *code, struct codegen_value *reg1,
                   struct codegen_value *reg2)
{
    generate_opcode(code, 0x85, reg1, reg2);
}

size_t generate_jump_if_zero(struct bytecode_array *code)
{
    array_macro_append_v(code, 0x74, 0xFF);
    return code->num - 1;
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

struct variable *codegen_variable_lookup(struct bytecode_array *code,
                                         const char *name)
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
        retval = lhs;

    case TOKEN_EQUALS:;
        struct codegen_value *out = raise_sp(code);
        generate_mov(code, out,
                     &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, 0});
        logd("lhs:%d, rhs:%d", lhs->type, rhs->type);
        generate_cmp(code, lhs, rhs);
        generate_setz(code, out);

        retval = out;
        break;

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
            struct variable *var =
                codegen_variable_lookup(code, expr->value.varname);
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
        PANIC("unknown expression type %d", expr->type);
        break;
    }
}

struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr)
{

    struct codegen_value *result = generate_expr_impl(code, expr);

    /* reset stack offset */
    /* generate_add(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER,
     * 0b100}, */
    /*              &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, */
    /*                                      code->stack_offset - stack_start});
     */
    /* code->stack_offset = stack_start; */

    return result;
}

void generate_return(struct bytecode_array *code)
{
    generate_add(
        code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
        &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, code->stack_offset});
    array_macro_append_v(code, 0xc3);
}

void generate_statement(struct bytecode_array *code,
                        struct ast_statement *statement);

void generate_block(struct bytecode_array *code, struct list *statements_head)
{

    struct list *pos;
    list_for_each(pos, statements_head)
    {
        struct ast_statement *s = containerof(pos, struct ast_statement, list);
        generate_statement(code, s);
    }
}

void generate_statement(struct bytecode_array *code,
                        struct ast_statement *statement)
{
    switch (statement->type)
    {
    case AST_STATEMENT_EXPR:
        generate_expr(code, statement->expr);
        break;
    case AST_STATEMENT_IF:;
        struct codegen_value *if_value =
            generate_expr(code, statement->block_expr);
        generate_mov(code, &immediate_temp_reg, if_value);
        generate_test(code, &immediate_temp_reg, &immediate_temp_reg);
        size_t jump_offset_location = generate_jump_if_zero(code);
        size_t start_size = code->num;
        generate_block(code, &statement->block);
        size_t end_size = code->num;

        logd("jumping over block of size:%p", end_size - start_size);

        ASSERT(end_size - start_size < 128, "cant encode displacement > 128");
        *(code->items + jump_offset_location) = end_size - start_size;

        break;
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
    generate_block(code, &function->function->block);
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
