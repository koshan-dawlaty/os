#include <container_of.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>

struct ir_variable *ir_variable_alloc(const char *name)
{
    struct ir_variable *out = malloc(sizeof(struct ir_variable));
    out->name = strdup(name);
    out->max_value = NULL;
    return out;
}

struct ir_variable *ir_variable_lookup(struct ir_program *program,
                                       const char *name)
{
    for (size_t i = 0; i < program->variables.num; i++)
    {
        struct ir_variable *var = program->variables.items[i];
        if (!strcmp(var->name, name))
        {
            return var;
        }
    }

    return NULL;
}

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
struct ir_value *ir_value_alloc_label(struct ir_program *program,
                                      struct ir_operation *next)
{
    struct ir_value *out = malloc(sizeof(struct ir_value));
    out->type = IR_VALUE_LABEL;
    out->label.next = next;
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
    case IR_OP_RET:
        return "RET";
    case IR_OP_MUL:
        return "MUL";
    case IR_OP_INVALID:
        return "INVALID";
    case IR_OP_JZ:
        return "JNZ";
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
    out->is_label = false;
    return out;
}

struct ir_operation *ir_operation_alloc_label(struct ir_value *label)
{
    struct ir_operation *out = malloc(sizeof(struct ir_operation));
    out->label.value = label;
    out->is_label = true;
    out->list = LIST_INIT();
    return out;
}

void ir_operation_append(struct ir_program *program, struct ir_operation *op)
{
    list_push_end(&op->list, &program->operations);
}

struct ir_value *ast_value_convert_ir(struct ir_program *program,
                                      struct ast_value value)
{
    if (value.is_var)
    {
        struct ir_variable *var = ir_variable_lookup(program, value.varname);

        NOT_NULL(var, "undeclared variable %s", value.varname);
        return var->max_value;
    }
    else
    {
        return ir_value_alloc_immediate(program, value.literal.integer);
    }
}

struct ir_value *ir_generate_expr_impl(struct ir_program *program,
                                       struct ast_expr *expr)
{
    switch (expr->type)
    {
    case AST_EXPRESSION_VALUE:;
        return ast_value_convert_ir(program, expr->value);
    case AST_EXPRESSION_OPERATOR:;

        struct ir_value *out = ir_value_alloc_reg(program);
        struct ir_value *lhs = ir_generate_expr_impl(program, expr->lhs);
        struct ir_value *rhs = ir_generate_expr_impl(program, expr->rhs);

        enum ir_operation_type op_type = IR_OP_INVALID;
        switch (expr->operator)
        {
        case TOKEN_PLUS:
            op_type = IR_OP_ADD;
            break;
        case TOKEN_MINUS:
            op_type = IR_OP_SUB;
            break;
        case TOKEN_STAR:
            op_type = IR_OP_MUL;
            break;

        /* might not work */
        case TOKEN_ASSIGN:
            op_type = IR_OP_MOV;
            break;

        default:
            PANIC("invalid operator token:%s", token_type_str(expr->operator));
        }
        struct ir_operation *op = ir_operation_alloc(op_type, out, lhs, rhs);
        ir_operation_append(program, op);

        return out;
    default:
        PANIC("unhandled expr type:%d", expr->type);
    }
}

struct ir_value *ir_generate_expr(struct ir_program *program,
                                  struct ast_expr *expr)
{
    if (expr->type == AST_EXPRESSION_VALUE)
    {
        struct ir_value *expr_value =
            ast_value_convert_ir(program, expr->value);
        struct ir_value *dest_value = ir_value_alloc_reg(program);
        ir_operation_append(program, ir_operation_alloc(IR_OP_MOV, dest_value,
                                                        expr_value, NULL));

        return dest_value;
    }
    else
    {
        return ir_generate_expr_impl(program, expr);
    }
}

void ir_generate_expr_assign(struct ir_program *program, struct ast_expr *expr)
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

    struct ir_variable *var =
        ir_variable_lookup(program, expr->lhs->value.varname);
    struct ir_value *lhs = ast_value_convert_ir(program, expr->lhs->value);
    struct ir_value *result = ir_generate_expr(program, expr->rhs);
    var->max_value = result;
}

void ir_generate_block(struct ir_program *program, struct list *statements)
{

    struct list *pos;
    list_for_each(pos, statements)
    {
        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);
        switch (statement->type)
        {
        case AST_STATEMENT_EXPR:
            ir_generate_expr_assign(program, statement->expr);

            break;
        case AST_STATEMENT_IF:;
            struct ir_value *if_val =
                ir_generate_expr(program, statement->block_expr);
            struct ir_operation *jump_op =
                ir_operation_alloc(IR_OP_JZ, if_val, NULL, NULL);
            ir_operation_append(program, jump_op);
            ir_generate_block(program, &statement->block);
            struct ir_value *label_value =
                ir_value_alloc_label(program, IR_PROGRAM_CURRENT_OP(program));
            logd("adding");
            list_add_before(&ir_operation_alloc_label(label_value)->list,
                            program->operations.prev);
            logd("done");
            /* ir_operation_append(program,
             * ir_operation_alloc_label(label_value)); */
            jump_op->two = label_value;

            break;
        case AST_STATEMENT_WHILE:
        case AST_STATEMENT_FOR:
            break;
        case AST_STATEMENT_DECLARE:;
            struct ir_variable *var = ir_variable_alloc(statement->varname);
            array_macro_append(&program->variables, var);
            struct ir_value *val =
                ir_generate_expr(program, statement->init_value);
            var->max_value = val;
            break;
        case AST_STATEMENT_RETURN:;
            /* TODO: there is probably a better way of doing this than
             * allocating a new register, there could be a special return value
             * register that would be automatically mapped to the architecture
             * return value */

            struct ir_value *return_dest =
                ir_generate_expr(program, statement->expr);
            ir_operation_append(
                program,
                ir_operation_alloc(IR_OP_RET, return_dest, NULL, NULL));
            break;
        }
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
        case IR_VALUE_LABEL:
            log_multiline_start(level);
            log_level(*level, "label(");
            log_level(*level, "%s",
                      ir_operation_type_str(value->label.next->type));
            log_level(*level, ")");
            log_multiline_end(level);
            break;
        }
    }
}

void ir_operation_log(struct ir_operation *op, struct log_level *level)
{
    log_multiline_start(level);

    if (!op->is_label)
    {

        log_level(*level, "%s", ir_operation_type_str(op->type));

        log_level(*level, " ");
        ir_value_log(op->one, level);
        log_level(*level, ", ");
        ir_value_log(op->two, level);
        log_level(*level, ", ");
        ir_value_log(op->three, level);
    }
    else
    {
        log_level(*level, "(");
        ir_value_log(op->label.value, level);
        log_level(*level, ")");
    }
    log_level(*level, "\n");

    log_multiline_end(level);
}

void ir_display(struct ir_program *program, struct log_level *level)
{
    log_multiline_start(level);
    struct list *pos;
    list_for_each(pos, &program->operations)
    {
        struct ir_operation *op = containerof(pos, struct ir_operation, list);
        ir_operation_log(op, level);
    }
    log_level(*level, "\n");
    log_multiline_end(level);
}
