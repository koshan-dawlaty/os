#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <lib/compiler/ast.h>
#include <types.h>

struct ir_label
{
    struct ir_instruction *instruction;
};

struct ir_program
{
    struct list instructions;
    size_t register_offset;
    size_t current_scope_depth;

    struct
    {
        struct ir_label *items;
        size_t num;
        size_t capacity;
    } labels;

    struct
    {
        struct ir_variable **items;
        size_t num;
        size_t capacity;
    } variables;
};

#define IR_PROGRAM_CURRENT_INS(program)                                        \
    (containerof(((program)->instructions.prev), struct ir_instruction, list))

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
            size_t label_num;
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
    struct ir_operation *assign_op;
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

struct ir_phi_reg
{
    struct ir_value *reg;
    /* the operation at the assignment to this register */
    struct ir_operation *op;
};

struct ir_operation
{
    union
    {
        struct
        {
            enum ir_operation_type type;
            struct ir_value *one, *two, *three;
        };

        struct
        {
            struct ir_value *dest;
            /* FIXME: this should probably be a list/array, which is why this is
             * not an ir_operation_type  */
            struct ir_phi_reg one;
            struct ir_phi_reg two;
        } phi;
    };

    bool is_phi;
};

struct ir_instruction
{
    union
    {
        struct ir_operation op;

        /* this maybe does not need to be here, but it makes printing /
         * debugging easier */
        struct
        {
            size_t label_num;
        } label;
    };

    struct list list;
    bool is_label;
};


struct ir_value *ir_generate_expr(struct ir_program *program,
                                  struct ast_expr *expr);

void ir_display(struct ir_program *program, struct log_level *level);
void ir_value_log(struct ir_value *value, struct log_level *level);
void ir_operation_log(struct ir_operation *op, struct log_level *level);

void ir_generate_block(struct ir_program *program, struct list *statements);
void ir_remove_phi(struct ir_program *program);

struct cfg_node
{
    struct
    {
        struct ast_statement *items;
        size_t num;
        size_t capacity;
    } statements;

    struct
    {
        struct cfg_node **items;
        size_t num;
        size_t capacity;
    } children;

    /* having pointers to the parent of each node might be unnecessary, this is
     * used for getting dominance frontier, you can maybe iterate backwards
     * through all nodes and see the predecessors that way? */
    /* immediate predecessors */
    struct
    {
        struct cfg_node **items;
        size_t num;
        size_t capacity;
    } parents;

    /* this maybe does not need to be its own array, you could do this as a
     * tree, and it would definitly be faster */
    struct
    {
        struct cfg_node **items;
        size_t num;
        size_t capacity;
    } dominance;

    struct
    {
        struct cfg_node **items;
        size_t num;
        size_t capacity;
    } dominance_frontier;
};
struct cfg_parser
{
    struct
    {
        struct cfg_node **items;
        size_t num;
        size_t capacity;
    } nodes;
};

struct cfg_node *cfg_parse(struct cfg_parser *parser, struct list *statements);
void cfg_display(struct cfg_node *root);
void cfg_set_dominance(struct cfg_parser *parser, struct cfg_node *root);
void cfg_set_dominance_frontier(struct cfg_parser *parser);

#endif
