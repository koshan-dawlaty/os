#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <lib/compiler/ast.h>
#include <types.h>

struct ir_operation;
struct ir_program
{
    struct list operations;
    size_t register_offset;
    size_t current_scope_depth;

    struct
    {
        struct ir_variable **items;
        size_t num;
        size_t capacity;
    } variables;

    /* phi operations that should be added at the end of this block */
    struct
    {
        struct ir_operation **items;
        size_t num;
        size_t capacity;
    } phi_op_queue;
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

struct ir_variable_version
{
    struct ir_value *var;
    size_t scope_depth;
};

struct ir_variable
{
    char *name;
    struct
    {
        struct ir_variable_version *items;
        size_t num;
        size_t capacity;
    } values;
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

    /* jump to two if one is zero */
    IR_OP_JZ,

    /* unconditionally jump to one */
    IR_OP_JMP,

    /* set one to two == three */
    IR_OP_SET_EQ,
    /* set one to two <= three */
    IR_OP_SET_LE,

    /* set one to true if two is not zero */
    IR_OP_SET_NZ,

    IR_OP_INVALID,
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
        struct
        {
            struct ir_value *dest;
            /* FIXME: this should probably be a list/array, which is why this is
             * not an ir_operation_type  */
            struct ir_value *one;
            struct ir_value *two;
        } phi;
    };

    /* phi function, described here
       https://en.wikipedia.org/wiki/Static_single-assignment_form */
    bool is_phi;

    bool is_label;
};

struct ir_value *ir_generate_expr(struct ir_program *program,
                                  struct ast_expr *expr);

void ir_display(struct ir_program *program, struct log_level *level);
void ir_value_log(struct ir_value *value, struct log_level *level);
void ir_operation_log(struct ir_operation *op, struct log_level *level);

void ir_generate_block(struct ir_program *program, struct list *statements);

#endif
