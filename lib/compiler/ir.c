#include <container_of.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/ast.h>
#include <lib/compiler/ir.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>

struct ir_variable *ir_variable_alloc(const char *name)
{
    struct ir_variable *out = malloc(sizeof(struct ir_variable));
    out->name = strdup(name);
    out->values.capacity = 0;
    out->values.num = 0;
    out->values.items = NULL;
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
        return "JZ";
    case IR_OP_SET_EQ:
        return "SET EQ";
    case IR_OP_SET_LE:
        return "SET LE";
    case IR_OP_SET_NZ:
        return "SET NZ";
    case IR_OP_JMP:
        return "JMP";
    default:
        return "UNKNOWN";
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
    out->is_phi = false;
    return out;
}

struct ir_operation *ir_operation_alloc_phi(struct ir_value *dest,
                                            struct ir_phi_reg one,
                                            struct ir_phi_reg two)
{
    struct ir_operation *out = malloc(sizeof(struct ir_operation));
    out->phi.dest = dest;
    out->phi.one = one;
    out->phi.two = two;
    out->is_label = false;
    out->is_phi = true;
    out->list = LIST_INIT();
    return out;
}

struct ir_operation *ir_operation_alloc_label(struct ir_value *label)
{
    struct ir_operation *out = malloc(sizeof(struct ir_operation));
    out->label.value = label;
    out->is_label = true;
    out->is_phi = false;
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
        struct ir_variable_version prev_version =
            var->values.items[var->values.num - 1];

        return prev_version.var;
    }
    else
    {
        return ir_value_alloc_immediate(program, value.literal.integer);
    }
}

struct ir_value *ir_generate_label(struct ir_program *program)
{
    struct ir_value *label_value =
        ir_value_alloc_label(program, IR_PROGRAM_CURRENT_OP(program));
    struct ir_operation *label_op = ir_operation_alloc_label(label_value);
    list_push_end(&label_op->list, &program->operations);
    return label_value;
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
            ir_operation_append(program,
                                ir_operation_alloc(IR_OP_ADD, out, lhs, rhs));
            break;
        case TOKEN_MINUS:
            ir_operation_append(program,
                                ir_operation_alloc(IR_OP_SUB, out, lhs, rhs));
            break;
        case TOKEN_STAR:
            ir_operation_append(program,
                                ir_operation_alloc(IR_OP_MUL, out, lhs, rhs));
            break;
        case TOKEN_LESS_EQUAL:
            ir_operation_append(
                program, ir_operation_alloc(IR_OP_SET_LE, out, lhs, rhs));
            break;

        /* might not work */
        case TOKEN_ASSIGN:
            ir_operation_append(program,
                                ir_operation_alloc(IR_OP_MOV, out, lhs, rhs));
            break;

        case TOKEN_EQUALS:
            ir_operation_append(
                program, ir_operation_alloc(IR_OP_SET_EQ, out, lhs, rhs));
            break;
        case TOKEN_AND:
            /* this could be done by codegen, and there might be more
             * optimizations that could be done there */

            ir_operation_append(
                program,
                ir_operation_alloc(IR_OP_MOV, out,
                                   ir_value_alloc_immediate(program, 0), NULL));

            struct ir_operation *jump_op =
                ir_operation_alloc(IR_OP_JZ, lhs, NULL, NULL);
            ir_operation_append(program, jump_op);

            ir_operation_append(
                program, ir_operation_alloc(IR_OP_SET_NZ, out, rhs, NULL));

            jump_op->two = ir_generate_label(program);
            break;

        default:
            PANIC("invalid operator token:%s", token_type_str(expr->operator));
        }

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
    logd("assign");
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
    struct ir_operation *result_op =
        containerof(program->operations.prev, struct ir_operation, list);

    struct ir_variable_version version = (struct ir_variable_version){
        result_op, result, program->current_scope_depth};
    array_macro_append(&var->values, version);

    /* FIXME: probably factor out */
    for (size_t i = 0; i < var->values.num - 1; i++)
    {
        struct ir_variable_version current_value = var->values.items[i];
        logd("version of var has register:%d", current_value.var->var.num);
        /* FIXME: this way of check if a variable needs to be reassigned is bad
         */
        if (current_value.scope_depth != program->current_scope_depth)
        {
            struct ir_value *new_value = ir_value_alloc_reg(program);
            struct ir_phi_reg phi_reg1 =
                (struct ir_phi_reg){.op = result_op, .reg = result};
            struct ir_phi_reg phi_reg2 = (struct ir_phi_reg){
                .op = current_value.assign_op, .reg = current_value.var};

            struct ir_operation *phi_op =
                ir_operation_alloc_phi(new_value, phi_reg1, phi_reg2);

            struct ir_variable_version new_var = {phi_op, new_value,
                                                  program->current_scope_depth};
            array_macro_append(&var->values, new_var);
            array_macro_append(&program->phi_op_queue_end, phi_op);
            break;
        }
    }
}

void ir_empty_phi_queue(struct ir_program *program)
{
    for (size_t i = 0; i < program->phi_op_queue_end.num; i++)
    {
        struct ir_operation *op = program->phi_op_queue_end.items[i];
        ir_operation_append(program, op);
    }

    program->phi_op_queue_end.capacity = 0;
    program->phi_op_queue_end.num = 0;
    free(program->phi_op_queue_end.items);
    program->phi_op_queue_end.items = NULL;
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

            program->current_scope_depth++;

            struct ir_operation *jump_op =
                ir_operation_alloc(IR_OP_JZ, if_val, NULL, NULL);
            ir_operation_append(program, jump_op);

            ir_generate_block(program, &statement->block);
            jump_op->two = ir_generate_label(program);

            program->current_scope_depth--;
            ir_empty_phi_queue(program);

            break;
        case AST_STATEMENT_WHILE:;

            struct ir_value *loop_start = ir_generate_label(program);
            program->current_scope_depth++;

            struct ir_value *condition =
                ir_generate_expr(program, statement->block_expr);

            struct ir_operation *cond_jump_op =
                ir_operation_alloc(IR_OP_JZ, condition, NULL, NULL);
            ir_operation_append(program, cond_jump_op);
            ir_generate_block(program, &statement->block);

            ir_operation_append(
                program, ir_operation_alloc(IR_OP_JMP, loop_start, NULL, NULL));
            cond_jump_op->two = ir_generate_label(program);

            program->current_scope_depth--;
            ir_empty_phi_queue(program);

            break;
        case AST_STATEMENT_FOR:
            break;
        case AST_STATEMENT_DECLARE:;
            struct ir_variable *var = ir_variable_alloc(statement->varname);
            array_macro_append(&program->variables, var);
            struct ir_value *val =
                ir_generate_expr(program, statement->init_value);

            struct ir_variable_version version =
                (struct ir_variable_version){IR_PROGRAM_CURRENT_OP(program),
                                             val, program->current_scope_depth};
            array_macro_append(&var->values, version);
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

void ir_phi_convert_mov(struct ir_program *program, struct ir_operation *phi)
{
    logd("%p, %p", phi->phi.one.op, phi->phi.two.op);
    struct ir_operation *one_mov_op =
        ir_operation_alloc(IR_OP_MOV, phi->phi.dest, phi->phi.one.reg, NULL);
    list_add_after(&one_mov_op->list, &phi->phi.one.op->list);

    logd("two op value:%p", phi->phi.two.op);
    logd("%d", phi->phi.two.op->is_label);
    logd("%p, %p", phi->phi.two.op->list.prev, phi->phi.two.op->list.next);
    struct ir_operation *two_mov_op =
        ir_operation_alloc(IR_OP_MOV, phi->phi.dest, phi->phi.two.reg, NULL);
    list_add_after(&two_mov_op->list, &phi->phi.two.op->list);
}

void ir_remove_phi(struct ir_program *program)
{
    struct list *pos;
    list_for_each(pos, &program->operations)
    {
        struct ir_operation *op = containerof(pos, struct ir_operation, list);
        if (op->is_phi)
        {
            ir_phi_convert_mov(program, op);
        }
    }
}

void ir_operation_log(struct ir_operation *op, struct log_level *level)
{
    log_multiline_start(level);

    if (op->is_phi)
    {

        log_level(*level, "PHI ");
        ir_value_log(op->phi.dest, level);
        log_level(*level, " (");
        ir_value_log(op->phi.one.reg, level);
        log_level(*level, " ");
        ir_value_log(op->phi.two.reg, level);
        log_level(*level, ")");
    }
    else if (op->is_label)
    {
        log_level(*level, " (");
        ir_value_log(op->label.value, level);
        log_level(*level, ")");
    }
    else
    {
        log_level(*level, "%s", ir_operation_type_str(op->type));

        log_level(*level, " ");
        ir_value_log(op->one, level);
        log_level(*level, ", ");
        ir_value_log(op->two, level);
        log_level(*level, ", ");
        ir_value_log(op->three, level);
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

struct cfg_node *cfg_node_alloc()
{
    struct cfg_node *out = malloc(sizeof(struct cfg_node));
    out->children.items = NULL;
    out->children.capacity = 0;
    out->children.num = 0;

    out->statements.items = NULL;
    out->statements.capacity = 0;
    out->statements.num = 0;

    return out;
}

struct cfg_node *cfg_parse(struct list *statements)
{
    struct cfg_node *out = cfg_node_alloc();
    struct list *pos;
    list_for_each(pos, statements)
    {
        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);

        switch (statement->type)
        {
        case AST_STATEMENT_IF:;
            struct cfg_node *child = cfg_parse(&statement->block);
            array_macro_append(&out->children, child);

            LIST_DEFINE_HEAD(new_head);
            new_head.next = pos->next;
            new_head.prev = statements->prev;
            new_head.next->prev = &new_head;
            new_head.prev->next = &new_head;

            statements->prev = pos;
            pos = statements;
            pos->next = statements;

            struct cfg_node *block_after = cfg_parse(&new_head);
            array_macro_append(&child->children, block_after);
            array_macro_append(&out->children, block_after);
            break;

        case AST_STATEMENT_EXPR:
        case AST_STATEMENT_DECLARE:
        case AST_STATEMENT_RETURN:
            /* derefrence might be stupid */
            array_macro_append(&out->statements, *statement);
            break;
        case AST_STATEMENT_WHILE:
        case AST_STATEMENT_FOR:
            PANIC("unimplemented");
        }
    }
    return out;
}

void cfg_display(struct cfg_node *node)
{
    logd("cfg block start");
    for (size_t i = 0; i < node->statements.num; i++)
    {
        struct ast_statement *statement = &array_index(&node->statements, i);
        logd("statement:%s", ast_statement_type_str(statement->type));
    }

    for (size_t i = 0; i < node->children.num; i++)
    {
        cfg_display(array_index(&node->children, i));
    }
    logd("cfg block end");
}
