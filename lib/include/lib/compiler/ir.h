#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <lib/compiler/ast.h>
#include <types.h>

struct ir_operation;
struct ir_program
{
    struct list operations;
    size_t register_offset;

    struct
    {
        struct ir_variable **items;
        size_t num;
        size_t capacity;
    } variables;
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

struct ir_variable
{
    char *name;
    struct ir_value *value;
};

struct ir_variable *ir_variable_alloc(const char *name, struct ir_value *val);

enum ir_operation_type
{
    /* one = two (operator) three */
    IR_OP_ADD,
    IR_OP_SUB,

    /* one = two */
    IR_OP_MOV,

	/* return value = one */
	IR_OP_RET,
};

struct ir_operation
{

    enum ir_operation_type type;
    struct ir_value *one, *two, *three;

    struct list list;
};

void ir_generate_expr(struct ir_program *program, struct ast_expr *expr);
void ir_display(struct ir_program *program, struct log_level *level);
void ir_generate_block(struct ir_program *program, struct list *statements);

#endif
