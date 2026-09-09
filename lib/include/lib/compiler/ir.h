#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <lib/compiler/ast.h>
#include <types.h>

struct ir_variable
{
};

struct ir_operation;

struct ir_program
{
    struct list operations;
    size_t register_offset;
};

struct ir_value
{
    union
    {
        struct
        {
            size_t num;
        } var;
        u64 value;
    };

    enum
    {
        IR_VALUE_IMMEDIATE,
        IR_VALUE_REIGSTER,
    } type;
};

enum ir_operation_type
{
    /* one = two (operator) three */
    IR_OP_ADD,
    IR_OP_SUB,

    /* one = two */
    IR_OP_MOV,
};

struct ir_operation
{

    enum ir_operation_type type;
    struct ir_value *one, *two, *three;

    struct list list;
};

void ir_generate_expr(struct ir_program *program, struct ast_expr *expr);
void ir_display(struct ir_program *program, struct log_level *level);

#endif
