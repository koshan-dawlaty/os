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

void generate_function(struct bytecode_array *code,
                       struct ast_top_statement *function);
struct codegen_value *generate_expr(struct bytecode_array *code,
                                    struct ast_expr *expr);

int run_code(u8 *bytecode, size_t len);
void generate_return(struct bytecode_array *code);

#endif
