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

#define IR_PROGRAM_CURRENT_OP(program)                                         \
    (containerof(((program)->operations.prev), struct ir_operation, list))

struct ir_value
{
    union
    {
        struct
        {
            size_t num;
        } var;
        struct
        {
            /* the operation after this label */
            struct ir_operation *next;
        } label;
        u64 value;
    };

    enum
    {
        IR_VALUE_IMMEDIATE,
        IR_VALUE_REIGSTER,
        IR_VALUE_LABEL,
    } type;
};

struct ir_variable
{
    char *name;
    /* the highest register number that this is stored in, the latest value of
     * this variable */
    struct ir_value *max_value;
};

struct ir_variable *ir_variable_alloc(const char *name);

enum ir_operation_type
{
    /* one = two (operator) three */
    IR_OP_ADD,
    IR_OP_SUB,
    IR_OP_MUL,

    /* one = two */
    IR_OP_MOV,

    /* return value = one */
    IR_OP_RET,

    IR_OP_INVALID,

    /* jump to two if one is zero */
    IR_OP_JZ,

	/* set one to two == three */
	IR_OP_SET_EQ,

	/* set one to true if two is not zero */
	IR_OP_SET_NZ,
};

struct ir_operation
{
    union
    {
        struct
        {
            enum ir_operation_type type;
            struct ir_value *one, *two, *three;

            struct list list;
        };
        struct
        {
            struct ir_value *value;
        } label;
    };
    bool is_label;
};

struct ir_value *ir_generate_expr(struct ir_program *program,
                                  struct ast_expr *expr);

void ir_display(struct ir_program *program, struct log_level *level);
void ir_value_log(struct ir_value *value, struct log_level *level);
void ir_operation_log(struct ir_operation *op, struct log_level *level);

void ir_generate_block(struct ir_program *program, struct list *statements);

#endif
