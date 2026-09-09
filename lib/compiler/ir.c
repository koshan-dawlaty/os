#include <container_of.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <math.h>
#include <stdlib.h>
#include <types.h>

/* these maybe shouldn't be a pointer, but we could add types to ir_value in the
 * future */
struct ir_value *ir_value_alloc_reg(struct ir_program *program)
{
    struct ir_value *out = malloc(sizeof(struct ir_value));
    out->type = IR_VALUE_REIGSTER;
    out->var.num = program->register_offset;
    program->register_offset++;
    return out;
}
struct ir_value *ir_value_alloc_immediate(struct ir_program *program,
                                          ptr_t value)
{
    struct ir_value *out = malloc(sizeof(struct ir_value));
    out->type = IR_VALUE_IMMEDIATE;
    out->value = value;
    return out;
}

const char *ir_operation_type_str(enum ir_operation_type type)
{
    switch (type)
    {
    case IR_OP_ADD:
        return "ADD";
    case IR_OP_SUB:
        return "SUB";
    case IR_OP_MOV:
        return "MOV";
    }
}

struct ir_operation *ir_operation_alloc(enum ir_operation_type type,
                                        struct ir_value *one,
                                        struct ir_value *two,
                                        struct ir_value *three)
{
    struct ir_operation *out = malloc(sizeof(struct ir_operation));
    out->type = type;
    out->one = one;
    out->two = two;
    out->three = three;
    out->list = LIST_INIT();
    return out;
}

struct ir_value *ast_value_convert_ir(struct ir_program *program,
                                      struct ast_value value)
{
    if (value.is_var)
    {
        return ir_value_alloc_reg(program);
    }
    else
    {
        /* FIXME: needs to lookup varname */
        return ir_value_alloc_immediate(program, value.literal.integer);
    }
}

struct ir_value *ir_generate_expr_impl(struct ir_program *program,
                                       struct ir_value *dest,
                                       struct ast_expr *expr)
{
    switch (expr->type)
    {
    case AST_EXPRESSION_VALUE:;

        return ast_value_convert_ir(program, expr->value);
    case AST_EXPRESSION_OPERATOR:;
        struct ir_value *lhs = ir_generate_expr_impl(program, dest, expr->lhs);
        struct ir_value *rhs = ir_generate_expr_impl(program, dest, expr->rhs);

        struct ir_operation *op = ir_operation_alloc(IR_OP_ADD, dest, lhs, rhs);
        list_push_end(&op->list, &program->operations);

        return dest;
    default:
        PANIC("unhandled expr type:%d", expr->type);
    }
}

void ir_generate_expr(struct ir_program *program, struct ast_expr *expr)
{
    /* TODO: need to support statements like
     * 1 + 1
     * and
     * 1 + printf(...)
     */
    ASSERT(expr->type == AST_EXPRESSION_OPERATOR &&
               expr->operator == TOKEN_ASSIGN,
           "not implemented for expression statements");
    ASSERT(expr->lhs->type == AST_EXPRESSION_VALUE && expr->lhs->value.is_var,
           "complex left hand side of assignment not implemented");

    struct ir_value *lhs = ast_value_convert_ir(program, expr->value);
    ir_generate_expr_impl(program, lhs, expr->rhs);
}

void ir_parse_block(struct ir_program *program, struct list *statements)
{
    struct list *pos;
    list_for_each(pos, statements)
    {
        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);
    }
}

void ir_value_log(struct ir_value *value, struct log_level *level)
{
    if (value == NULL)
    {
        log_level(*level, "(null)");
    }
    else
    {
        switch (value->type)
        {
        case IR_VALUE_REIGSTER:
            log_level(*level, "r(%d)", value->var.num);
            break;
        case IR_VALUE_IMMEDIATE:
            log_level(*level, "%d", value->value);
            break;
        }
    }
}

void ir_display(struct ir_program *program, struct log_level *level)
{
    log_multiline_start(level);
    struct list *pos;
    list_for_each(pos, &program->operations)
    {
        struct ir_operation *op = containerof(pos, struct ir_operation, list);
        log_level(*level, "%s", ir_operation_type_str(op->type));

        log_level(*level, " ");
        ir_value_log(op->one, level);
        log_level(*level, ", ");
        ir_value_log(op->two, level);
        log_level(*level, ", ");
        ir_value_log(op->three, level);

        log_level(*level, "\n");
    }
    log_multiline_end(level);
}
