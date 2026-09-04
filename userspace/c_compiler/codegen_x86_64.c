#include <complex.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/c_compiler.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sys/syscall.h>
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
                                   struct codegen_value *src,
                                   struct codegen_value *dest)
{

    generate_mov(code, &immediate_temp_reg, src);
    generate_ax_opcode(code, 0xF7, 0b100, &immediate_temp_reg);
    /* ax */
    return codegen_value_alloc(CODEGEN_VALUE_REGISTER, 0);
}

struct codegen_value *generate_operator(struct bytecode_array *code,
                                        struct codegen_value *dest,
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
    case TOKEN_STAR:
        logd("star");
        if (lhs == NULL)
        {
            lhs = dest;
        }
        generate_mov(code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0},
                     lhs);
        struct codegen_value *value = generate_mul(code, rhs, dest);
        retval = generate_push(code, value);
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
            logd("immediate %d", expr->value.literal.integer);
            return codegen_value_alloc(CODEGEN_VALUE_IMMEDIATE,
                                       expr->value.literal.integer);
        }
        break;
    case AST_EXPRESSION_CALL:
        break;
    case AST_EXPRESSION_OPERATOR:;

        struct codegen_value *lhs = generate_expr(code, expr->lhs, dest);
        struct codegen_value *rhs = generate_expr(code, expr->rhs, dest);

        /* if (lhs != NULL && rhs != NULL) */
        /* { */
        /*
         * FIXME: this is kind of weird, it might be a good idea to split
         * this into multiple functions (similar to parsing expressions for
         * the ast)
         */
        return generate_operator(code, dest, lhs, rhs, expr->operator);
        /* } */

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
		
		/* (1 + ((2 + 3) + (4 + 5))) + 6 */
			/* 1 + 2 + 3 */

        // clang-format on
    );

    /* program = " 3 + 5 * 2 - (4 * 2) "; */
    /* program = " (3 + 5 * 2) - (7 - 2 * 6)"; */

    /* program = "3 - 5 * 2"; */
    /* program = " (3 - 5 * 2) - (2 - 1)"; */
    /* program = " 3 * 2 - 5 * 2 "; */
    /* program = " 3 * 2 + 1 + 1 "; */

    /* program = "2 + 3 * 4 - 6 * 7 + 5 * 5 " */
    /*           "- 9 * 3 + 5 + 7 * 2 - 4 "; */
    /* program = "2 + 3 * 4 - 6 * 2 + 5 * 5 "; */
    /* program = "2 + 3 * 4 - 6 * 2 + 5"; */

    // program = "2 + 3 * 4 - 6 * 7 + 5 * 5 "
    //            "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8";

    /* "+ 10 * (2 + 3 * (4 + 5)) - 7 * 8 + 9" */
    /* "- 11 + 2 * 3 * (4 + 5) - 6 * (7 + 8)"; */

    // program = " 3 + 5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
    //            "+ 2 + 3 * 4 - 6 * 2 + 5 + 7 + 8 * 2"
    //            "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
    //            "+ 10 * (2 + 3 * (4 + 5)) - 7 * 8 + 9"
    //            "- 11 + 2 * 3 * (4 + 5) - 6 * (7 + 8)";
	
    program = "  5 * 2 - (7 - 2 * 6) + 8 * (4 + 1) - 9"
              "+ 2 + 3 * 4 - 6 * 2 + 5 + 7 + 8 * 2"
              "- 9 * 3 + 5 + 7 * 2 - 4 * (6 - 1) + 8"
              "+ 10 * (2 + 3 * (4 + 5)) ";

    tokenizer_parse_tokens(program, &tokens);

    LIST_DEFINE_HEAD(top_statements_head);

    struct ast_parser parser = {&tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_error *err;
    struct ast_expr *expr = ast_parse_expression(&parser, &err);

    struct bytecode_array code = {0};
    struct codegen_value dest = {CODEGEN_VALUE_REGISTER, 0b1000};

    generate_mov(&code, &dest,
                 &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, 0});

    struct codegen_value *result = generate_expr(&code, expr, &dest);
    logd("value, type: %d, %d", result->value, result->type);
    if (result != NULL)
    {
        generate_mov(&code, &dest, result);
    }

    generate_mov(&code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0},
                 &dest);

    logd("final stack offset:%d", code.stack_offset);
    /* reset stack offset */
    generate_add(
        &code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
        &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, code.stack_offset});

    array_macro_append_v(&code, 0xc3);

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

    exit(0);
}
