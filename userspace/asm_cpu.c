#include "lib/log.h"
#include <ctype.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <stdlib.h>
#include <string.h>

struct asm_instruction
{
    const char *opcode_str;

    struct
    {
        u8 opcode_num;
        /* currently unused */
        bool regs_only;
        bool two_args;
    } regs;

    struct
    {
        u8 opcode_num;
        /* currently unused */
        bool imm_only;
        bool two_args;
    } imm;
};

const struct asm_instruction instructions[] = {
    {"add", {0, false, false}, {0, false, false}},
    {"mov", {1, false, true}, {1, false, true}},
    {"sub", {2, false, false}, {2, false, false}},
};

const struct asm_instruction *lookup_instruction(const char *opcode_str)
{
    for (size_t i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++)
    {
        if (!strcmp(instructions[i].opcode_str, opcode_str))
        {
            return &instructions[i];
        }
    }
    return NULL;
}

struct asm_code
{
    u32 *items;
    size_t num;
    size_t capacity;
};

struct asm_argument
{
    union
    {
        ptr_t immediate;
        u8 reg;
    };

    enum asm_value_type
    {
        ASM_TYPE_IMMEDIATE,
        ASM_TYPE_REGISTER,
    } type;
};

/* maybe make, or find if it exists a standard library function for something
 * similar to this */
/* passing a pointer to the number of bytes used here is kind of stupid */
int str_to_int_limited(const char *str, size_t *bytes_used)
{
    size_t num = 0;
    size_t i = 0;
    while (str[i] >= '0' && str[i] <= '9')
    {
        num *= 10;
        num += str[i] - '0';

        i++;
    }

    *bytes_used += i;
    return num;
}

#define MAX_OPCODE_LEN 4

size_t asm_parse_argument(const char *str, struct asm_argument *arg)
{

    size_t bytes_used = 0;
    switch (*str)
    {
    case '$':
        bytes_used++;
        arg->type = ASM_TYPE_IMMEDIATE;
        arg->immediate = str_to_int_limited(str + 1, &bytes_used);
        break;
    case '%':
        bytes_used++;
        arg->type = ASM_TYPE_REGISTER;
        arg->reg = str_to_int_limited(str + 1, &bytes_used);
        break;

    default:
        PANIC("unknown start of value %c", *str);
    }
    return bytes_used;
}

/* FIXME: this is copied from the compiler codegen for this architecture, in
 * codegen_test123.c */
void asm_append_reg_instruction(struct asm_code *code, u8 opcode, u8 reg1,
                                u8 reg2, u8 reg3)
{
    u32 instruction = opcode | (reg1 << 6) | (reg2 << 11) | (reg3 << 16);
    array_macro_append_v(code, instruction);
}

void asm_append_imm_instruction(struct asm_code *code, u8 opcode, u8 dest,
                                u8 src, u16 immediate)
{
    u32 instruction =
        (opcode) | ((src) << 6) | ((dest) << 11) | (immediate << 16);
    array_macro_append_v(code, instruction);
}

size_t skip_spaces(const char *str)
{
    size_t i = 0;
    while (str[i] == ' ' || str[i] == '\t')
    {
        i++;
    }
    return i;
}

size_t assemble_instruction(const char *str, struct asm_code *code)
{
    char opcode_str[MAX_OPCODE_LEN] = {0};
    size_t i = 0;
    while (str[i] != '\n' && str[i] != ' ' && str[i])
    {
        ASSERT(i < MAX_OPCODE_LEN, "opcode too long");
        opcode_str[i] = str[i];

        i++;
    }

    /* TODO: check if this a no argument or 1 argument instruction, and handle
     * appropriately */
    const struct asm_instruction *inst = lookup_instruction(opcode_str);

    i += skip_spaces(str + i);
    struct asm_argument arg1;
    i += asm_parse_argument(str + i, &arg1);
    ASSERT(arg1.type != ASM_TYPE_IMMEDIATE, "can't have immediate as dest");

    i += skip_spaces(str + i);
    struct asm_argument arg2;
    i += asm_parse_argument(str + i, &arg2);

    if (arg2.type == ASM_TYPE_IMMEDIATE)
    {

        ASSERT(inst->imm.two_args, "need another register for the immediate "
                                   "version of this instruction");

        asm_append_imm_instruction(code, inst->imm.opcode_num, arg1.reg, 0,
                                   arg2.immediate);
    }
    else if (arg2.type == ASM_TYPE_REGISTER)
    {
        if (inst->regs.two_args)
        {
            asm_append_reg_instruction(code, inst->imm.opcode_num, arg1.reg, 0,
                                       arg2.reg);
        }
        else
        {
            i += skip_spaces(str + i);
            struct asm_argument arg3;
            i += asm_parse_argument(str + i, &arg3);

            if (arg3.type == ASM_TYPE_IMMEDIATE)
            {
                asm_append_imm_instruction(code, inst->imm.opcode_num, arg1.reg,
                                           arg2.reg, arg3.immediate);
            }
            else
            {
                asm_append_reg_instruction(code, inst->imm.opcode_num, arg1.reg,
                                           arg2.reg, arg3.reg);
            }
        }
    }
    else
    {
        PANIC("unknown enum value %d", arg2.type);
    }

    return i;
}

void assemble_program(const char *str, struct asm_code *code)
{
    while (*str != 0)
    {
        str += assemble_instruction(str, code);
        str++;
    }
}

int main(int argc, char **argv)
{
	logd("test");
    struct asm_code code = {0};
    if (argc <= 1)
    {
        // clang-format off
        assemble_program(
				"mov %1 $123\n"
				"add %1 %2 $123\n"
				"add %1 %2 %3\n"

				, &code);
        // clang-format on
    }
    else
    {
    }
    log_multiline_start(&LOG_DEBUG);
    for (size_t i = 0; i < code.num; i++)
    {
        logd("%p ", code.items[i]);
    }
    logd("\n");
    log_multiline_end(&LOG_DEBUG);
    return 0;
}
