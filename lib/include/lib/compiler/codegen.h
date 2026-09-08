#ifndef CODEGEN_H
#define CODEGEN_H

#include <lib/compiler/ast.h>
#include <types.h>

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

#define CODEGEN_VALUE_INIT(type, val) ((struct codegen_value){type, val})

void generate_function(struct bytecode_array *code,
                       struct ast_top_statement *function);
struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr);
void generate_mov(struct bytecode_array *code, struct codegen_value *dest,
                  struct codegen_value *src);

int run_code(u8 *bytecode, size_t len);
void generate_return(struct bytecode_array *code);

#endif
