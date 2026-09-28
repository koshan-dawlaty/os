#ifndef CODEGEN_H
#define CODEGEN_H

#include <lib/compiler/ast.h>
#include <lib/compiler/ir.h>
#include <types.h>

struct codegen_label
{
    /* absolute offset in the code of the jump */
    size_t jump_offset;
    /* absolute offset in the code of where the jump will go */
    size_t label_offset;

    size_t label_num;
};

struct bytecode_array
{
    u8 *items;
    size_t capacity;
    size_t num;

    struct
    {
        struct codegen_label *items;
        size_t capacity;
        size_t num;
    } labels;

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
        CODEGEN_VALUE_MEMORY_REG,
        CODEGEN_VALUE_MEMORY_STACK_REF,
        CODEGEN_VALUE_IMMEDIATE,
        CODEGEN_VALUE_STACK,
    } type;
    union
    {
        u64 value;
        u8 reg;
        union
        {
            u8 reg;
			u64 stack_offset;
        } memory;
    };
};

#define CODEGEN_VALUE_INIT(type, val) ((struct codegen_value){type, val})

void generate_function(struct bytecode_array *code,
                       struct ast_top_statement *function);
struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr);
void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src);

int run_code(u8 *bytecode, size_t len);
void generate_return(struct bytecode_array *code);

void codegen_generate_ir(struct bytecode_array *code,
                         struct ir_program *program);

#endif
