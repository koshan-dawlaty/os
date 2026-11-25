#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/codegen.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <tests/tests.h>
#include <unistd.h>
#include <userspace/mouse.h>
#include <utils.h>

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

void codegen_add_immediate(struct bytecode_array *code,
                           struct codegen_value *imm)
{
    if (imm->type == CODEGEN_VALUE_IMMEDIATE)
    {
        array_macro_append_buf(code, &imm->value, 4);
    }
}

u8 get_modrm_bytes(u8 mod, u8 rm)
{
    return 0b11000000 | (mod & 0b111) << 3 | BYTE_AS_RM(rm);
}

u8 get_modrm_rm(u8 mod, struct codegen_value *rm)
{
    if (rm->type == CODEGEN_VALUE_STACK)
    {
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
        return 0b01000000 | BYTE_AS_RM(0b100) | ((reg->value) << 3);
    }
    else if (rm->type == CODEGEN_VALUE_MEMORY)
    {
        return 0b00000000 | BYTE_AS_RM(rm->memory.reg) | ((reg->value) << 3);
    }
    else
    {
        return 0b11000000 | BYTE_AS_RM(rm->reg) | ((reg->value & 0b111) << 3);
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
        array_macro_append_v(code, 0b00100100,
                             code->stack_offset - memory->value);
        /* memory->value - code->stack_offset); */
    }
}

struct codegen_value immediate_temp_reg = {CODEGEN_VALUE_REGISTER, 1};
struct codegen_value immediate_temp_reg2 = {CODEGEN_VALUE_REGISTER, 3};
struct codegen_value stack_stack_temp_reg = {CODEGEN_VALUE_REGISTER, 0b110};

bool codegen_type_is_memory(enum codegen_value_type type)
{
    return type == CODEGEN_VALUE_STACK || type == CODEGEN_VALUE_MEMORY;
}

void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src);

void generate_add(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src);

void generate_opcode(struct bytecode_array *code, u8 opcode,
                     struct codegen_value *dest, struct codegen_value *src)
{

    if (codegen_type_is_memory(src->type) && codegen_type_is_memory(dest->type))
    {
        /* this might end up being annoying to debug because of recursion */
        generate_mov(code, &stack_stack_temp_reg, src);
        src = &stack_stack_temp_reg;
    }

    struct codegen_value *reg, *rm;
    if (dest->type == CODEGEN_VALUE_REGISTER &&
        src->type != CODEGEN_VALUE_REGISTER)
    {
        opcode |= 0b10; /* r/m to register */
                        /* struct codegen_value *temp = dest; */
                        /* dest = src; */
                        /* src = temp; */
        rm = src;
        reg = dest;
    }
    else
    {
        opcode &= ~0b10;
        rm = dest;
        reg = src;
    }

    generate_opcode_bytes(code, get_rex(reg, rm), opcode, get_modrm(reg, rm));

    generate_sib_byte(code, rm);
    codegen_add_immediate(code, src);
}

void generate_opcode_modrm(struct bytecode_array *code, u8 opcode, u8 modrm,
                           struct codegen_value *dest,
                           struct codegen_value *src)
{
    generate_opcode_bytes(code, get_rex(dest, src), opcode, modrm);

    generate_sib_byte(code, dest);
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
    codegen_add_immediate(code, src);
}

void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src)
{
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

    /* FIXME: these instructions only set the low byte, and i could not get it
     * to work with the 1 byte version of test/cmp, so this is here for now */
    generate_mov(code, dest, &CODEGEN_VALUE_INIT(CODEGEN_VALUE_IMMEDIATE, 0));
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

void generate_setle(struct bytecode_array *code, struct codegen_value *dest)
{
    generate_set_conditional(code, 0x9E, dest);
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
    /* array_macro_append_v(code, 0x74, 0); */
    array_macro_append_v(code, 0x0F, 0x84, 0, 0, 0, 0);
    return code->num - 4;
}
size_t generate_jump(struct bytecode_array *code)
{
    array_macro_append_v(code, 0xE9, 0, 0, 0, 0);
    return code->num - 4;
}

static void codegen_write_jump_offset(struct bytecode_array *code,
                                      size_t offset, u32 new_value)
{
    *(u32 *)(code->items + offset) = new_value;
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

void codegen_write_addr(struct bytecode_array *code, struct codegen_value *dest,
                        struct codegen_value *value)
{
    switch (value->type)
    {
    case CODEGEN_VALUE_MEMORY:
        generate_mov(code, dest, value);
        break;
    case CODEGEN_VALUE_STACK:
        generate_mov(code, dest,
                     &CODEGEN_VALUE_INIT(CODEGEN_VALUE_REGISTER, 0b100));
        logd("converting offset of %x to address", value->value);
        generate_add(code, dest,
                     &CODEGEN_VALUE_INIT(CODEGEN_VALUE_IMMEDIATE,
                                         code->stack_offset - value->value));
        break;
    default:
        PANIC("can't take address of that");
    }
}

struct codegen_value memory_temp_reg = {CODEGEN_VALUE_REGISTER, 0b111};

struct codegen_value *codegen_value_to_memory(struct bytecode_array *code,
                                              struct codegen_value *value)
{
    switch (value->type)
    {
    case CODEGEN_VALUE_REGISTER:
    case CODEGEN_VALUE_IMMEDIATE:
    case CODEGEN_VALUE_STACK:
        generate_mov(code, &memory_temp_reg, value);
        return codegen_value_alloc(CODEGEN_VALUE_MEMORY, memory_temp_reg.reg);
    default:
        UNREACHABLE("not implemented");
    }
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

void generate_return(struct bytecode_array *code)
{
    generate_add(
        code, &(struct codegen_value){CODEGEN_VALUE_REGISTER, 0b100},
        &(struct codegen_value){CODEGEN_VALUE_IMMEDIATE, code->stack_offset});
    array_macro_append_v(code, 0xc3);
}

struct codegen_value *codegen_convert_ir_value(struct bytecode_array *code,
                                               struct ir_value *value)
{
    if (value == NULL)
    {
        return NULL;
    }
    switch (value->type)
    {
    case IR_VALUE_IMMEDIATE:
        return codegen_value_alloc(CODEGEN_VALUE_IMMEDIATE, value->value);
    case IR_VALUE_REIGSTER:
        return codegen_value_alloc(CODEGEN_VALUE_STACK,
                                   code->stack_offset - (value->var.num * 8));
    case IR_VALUE_LABEL:
        /* labels should be handled by using the struct ir_value, codegen does
         * not need a value for them */
        return NULL;
    }
}

/* correctly sets jump offsets a label for an offset and an ir_value for a
 * label */
void codegen_add_jump_labels(struct bytecode_array *code, size_t label_num,
                             size_t offset)
{
    for (size_t i = 0; i < code->labels.num; i++)
    {
        struct codegen_label *l = &code->labels.items[i];
        logd("looking at label:%d", l->jump_offset);
        if (l->label_num == label_num)
        {
            logd("saw label before this");
            l->jump_offset = offset;
            return;
        }
    }
    struct codegen_label new_label = {offset, 0, label_num};
    array_macro_append(&code->labels, new_label);
}

void codegen_generate_op(struct bytecode_array *code, struct ir_operation *op)
{
    struct codegen_value *one = codegen_convert_ir_value(code, op->one);
    struct codegen_value *two = codegen_convert_ir_value(code, op->two);
    struct codegen_value *three = codegen_convert_ir_value(code, op->three);

    switch (op->type)
    {
        /* FIXME: this allocates many temporary variables, this would not be
         * necessary if the program was in ssa form */
    case IR_OP_ADD:;

        generate_mov(code, one, two);
        generate_add(code, one, three);
        break;
    case IR_OP_SUB:
        generate_mov(code, one, two);
        generate_sub(code, one, three);
        break;
    case IR_OP_MUL:
        generate_mov(code, one, two);
        struct codegen_value *out = generate_mul(code, one, three);
        generate_mov(code, one, out);
        break;
    case IR_OP_MOV:
        generate_mov(code, one, two);
        break;
    case IR_OP_RET:
        generate_mov(code, &CODEGEN_VALUE_INIT(CODEGEN_VALUE_REGISTER, 0), one);
        generate_return(code);
        break;
    case IR_OP_JZ:
        generate_test(code, one, one);
        size_t offset = generate_jump_if_zero(code);
        codegen_add_jump_labels(code, op->two->label.label_num, offset);

        break;
    case IR_OP_SET_EQ:
        generate_cmp(code, two, three);
        generate_setz(code, one);
        break;
    case IR_OP_SET_NZ:
        generate_test(code, two, two);
        generate_setnz(code, one);
        break;
    case IR_OP_SET_LE:
        generate_cmp(code, two, three);
        generate_setle(code, one);
        break;
    case IR_OP_STORE:
        ASSERT(two->type != CODEGEN_VALUE_IMMEDIATE,
               "not implemented for immediate");

        generate_mov(code, codegen_value_to_memory(code, one), two);
        break;
    case IR_OP_LOAD:
        ASSERT(two->type != CODEGEN_VALUE_IMMEDIATE,
               "not implemented for immediate");
        generate_mov(code, one, codegen_value_to_memory(code, two));
        break;
    case IR_OP_ADDR_OF:
        ASSERT(two->type != CODEGEN_VALUE_IMMEDIATE,
               "can't take address of that");
        codegen_write_addr(code, one, two);
        break;

    case IR_OP_JMP:
    {
        size_t offset = generate_jump(code);
        codegen_add_jump_labels(code, op->one->label.label_num, offset);
    }
    break;

    default:
    case IR_OP_INVALID:
        PANIC("invalid ir op");
    }
}

void codegen_generate_label(struct bytecode_array *code, size_t label_num)
{
    logd("label");
    for (size_t i = 0; i < code->labels.num; i++)
    {
        struct codegen_label *label = &code->labels.items[i];

        /* TODO: pointer comparing will probably work for right now, but in the
         * future a number system, similar to registers should probably be used
         */
        if (label->label_num == label_num)
        {
            label->label_offset = code->num - 4;
            return;
        }
    }
    struct codegen_label new_label = {0, code->num - 4, label_num};
    array_macro_append(&code->labels, new_label);
}

void codegen_generate_phi(struct bytecode_array *code,
                          struct ir_operation *phi_op)
{
}

void codegen_generate_ir(struct bytecode_array *code,
                         struct ir_program *program)
{
    size_t vars_size = program->register_offset * 8;
    generate_sub(code, &CODEGEN_VALUE_INIT(CODEGEN_VALUE_REGISTER, 0b100),
                 &CODEGEN_VALUE_INIT(CODEGEN_VALUE_IMMEDIATE, vars_size));
    code->stack_offset += vars_size;

    /* for (size_t i = 8; i < vars_size; i+=8) */
    /* { */
    /*     generate_mov(code, */
    /*                  &CODEGEN_VALUE_INIT(CODEGEN_VALUE_STACK, */
    /*                                      i), */
    /*                  &CODEGEN_VALUE_INIT(CODEGEN_VALUE_IMMEDIATE, 0)); */
    /* } */

    struct list *pos;
    list_for_each(pos, &program->instructions)
    {
        struct ir_instruction *inst =
            containerof(pos, struct ir_instruction, list);
        if (inst->is_label)
        {
            codegen_generate_label(code, inst->label.label_num);
        }
        else
        {
            if (inst->op.is_phi)
            {
                codegen_generate_phi(code, &inst->op);
            }
            else
            {
                codegen_generate_op(code, &inst->op);
            }
        }
    }
    generate_return(code);

    for (size_t i = 0; i < code->labels.num; i++)
    {
        struct codegen_label *label = &code->labels.items[i];
        logd("jump offset:%d, label offset:%d", label->jump_offset,
             label->label_offset);
        codegen_write_jump_offset(code, label->jump_offset,
                                  label->label_offset - label->jump_offset);
    }
}
