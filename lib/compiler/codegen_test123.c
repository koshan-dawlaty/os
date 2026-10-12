#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/codegen.h>
#include <lib/compiler/codegen_test123.h>
#include <lib/compiler/ir.h>
#include <stdlib.h>

struct test123_value
{
    enum test123_value_type
    {
        TEST123_VALUE_IMMEDIATE,
        TEST123_VALUE_REG,
    } type;
    u32 data;
};

struct test123_value *test123_value_alloc(enum test123_value_type type,
                                          u32 data)
{
    struct test123_value *out = malloc(sizeof(struct test123_value));
    out->type = type;
    out->data = data;
    return out;
}

struct test123_value *test123_value_convert_ir(struct ir_value *value)
{
    if (value == NULL)
    {
        return NULL;
    }

    switch (value->type)
    {
    case IR_VALUE_IMMEDIATE:
        return test123_value_alloc(TEST123_VALUE_IMMEDIATE, value->value);
    case IR_VALUE_REIGSTER:
        logd("allocating as %d", value->var.num);
        return test123_value_alloc(TEST123_VALUE_REG, value->var.num);
    case IR_VALUE_LABEL:
        return NULL;
    }
}

void test123_append_instruction(struct bytecode_array *code, u32 instruction)
{
    array_macro_append_v(code, (instruction), (instruction >> 8),
                         (instruction >> 16), (instruction >> 24));
}

void test123_generate_instruction_bytes(struct bytecode_array *code, u8 opcode,
                                        u8 reg1, u8 reg2, u8 reg3)
{
    // clang-format off
	u32 instruction = opcode | (reg1 << 6) | (reg2 << 11) | (reg3 << 16);
	logd("0x%p", instruction);
    // clang-format on
    test123_append_instruction(code, instruction);
}

void test123_generate_instruction_immediate_bytes(struct bytecode_array *code,
                                                  u8 opcode, u8 dest, u8 src,
                                                  u16 immediate)
{
    logd("r1:%d", src);
    logd("opcode:%d", opcode);
    /* maybe and with mask so extra bits are not included */
    u32 instruction =
        (opcode) | ((src) << 6) | ((dest) << 11) | (immediate << 16);
    logd("0x%p", instruction);

    test123_append_instruction(code, instruction);
}

void test123_generate_instruction(struct bytecode_array *code, u8 opcode,
                                  struct test123_value *one,
                                  struct test123_value *two,
                                  struct test123_value *three)
{

    if (three == NULL)
    {
        if (two->type == TEST123_VALUE_IMMEDIATE)
        {
            test123_generate_instruction_immediate_bytes(
                code, (opcode << 1) | 1, one->data, 0, two->data);
        }
        else
        {
            test123_generate_instruction_bytes(code, (opcode << 1) & ~1,
                                               two->data, 0, one->data);
        }
    }
    else if (three->type == TEST123_VALUE_IMMEDIATE)
    {
        test123_generate_instruction_immediate_bytes(
            code, (opcode << 1) | 1, one->data, two->data, three->data);
    }
    else if (two->type == TEST123_VALUE_IMMEDIATE)
    {
        test123_generate_instruction_immediate_bytes(
            code, (opcode << 1) | 1, one->data, three->data, two->data);
    }
    else
    {
        logd("three registers %d %d %d", two->data, three->data, one->data);
        test123_generate_instruction_bytes(code, (opcode << 1) & ~1, two->data,
                                           three->data, one->data);
    }
}

void test123_generate_mov(struct bytecode_array *code,

                          struct test123_value *dest, struct test123_value *src)
{
    test123_generate_instruction(code, 1, dest, src, NULL);
}

/*
 * moves the first argument into the destination, then applies the second
 * argument with the destination register
 */
void test123_generate_instruction_imm_mov(struct bytecode_array *code,
                                          u8 opcode, struct test123_value *dest,
                                          struct test123_value *one,
                                          struct test123_value *two)
{
    if (one->type == TEST123_VALUE_IMMEDIATE &&
        two->type == TEST123_VALUE_IMMEDIATE)
    {
        test123_generate_mov(code, dest, one);
        test123_generate_instruction(code, opcode, dest, dest, two);
    }
    else
    {
        test123_generate_instruction(code, opcode, dest, one, two);
    }
}

#define TEST123_TEMP_REG_1                                                     \
    ((struct test123_value){.type = TEST123_VALUE_REG, .data = 20})

void test123_generate_store(struct bytecode_array *code,
                            struct test123_value *dest,
                            struct test123_value *src)
{
    ASSERT(dest->type != TEST123_VALUE_IMMEDIATE,
           "not implemented for immediate address");
    if (src->type == TEST123_VALUE_IMMEDIATE)
    {
        test123_generate_mov(code, &TEST123_TEMP_REG_1, src);
        src = &TEST123_TEMP_REG_1;
    }
    test123_generate_instruction_immediate_bytes(code, 49, src->data,
                                                 dest->data, 0);
}

void test123_generate_op(struct bytecode_array *code, struct ir_operation *op)
{
    struct test123_value *one = test123_value_convert_ir(op->one);
    struct test123_value *two = test123_value_convert_ir(op->two);
    struct test123_value *three = test123_value_convert_ir(op->three);

    switch (op->type)
    {
    case IR_OP_ADD:
        test123_generate_instruction_imm_mov(code, 0, one, two, three);
        break;
    case IR_OP_MOV:
        /* might not work if three needs to have the value */
        test123_generate_mov(code, one, two);
        break;
    case IR_OP_SUB:
        test123_generate_instruction_imm_mov(code, 2, one, two, three);
        break;
    case IR_OP_MUL:
        test123_generate_instruction_imm_mov(code, 3, one, two, three);
        break;

    case IR_OP_RET:
        test123_generate_instruction_bytes(code, 63, 0, 0, 0);
        break;

    case IR_OP_STORE:
        test123_generate_store(code, one, two);
        break;

    case IR_OP_LOAD:
        test123_generate_instruction_immediate_bytes(code, 33, two->data,
                                                     one->data, 0);
        break;

    default:
        PANIC("unknown op for test123 %d", op->type);
    }
}

void test123_generate_ir(struct bytecode_array *code,
                         struct ir_program *program)
{

    struct list *pos;
    list_for_each(pos, &program->instructions)
    {
        struct ir_instruction *inst =
            containerof(pos, struct ir_instruction, list);
        if (inst->is_label)
        {
            PANIC("cant do label yet");
        }
        else
        {
            if (inst->op.is_phi)
            {
                PANIC("dont use");
            }
            else
            {
                test123_generate_op(code, &inst->op);
            }
        }
    }
}
