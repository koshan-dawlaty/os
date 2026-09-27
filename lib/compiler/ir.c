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
                                      struct ir_instruction *inst)
{
    struct ir_value *out = malloc(sizeof(struct ir_value));
    out->type = IR_VALUE_LABEL;
    array_macro_append(&program->labels, (struct ir_label){inst});
    out->label.label_num = program->labels.num - 1;
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

struct ir_instruction *ir_inst_alloc_op(enum ir_operation_type type,
                                        struct ir_value *one,
                                        struct ir_value *two,
                                        struct ir_value *three)
{
    struct ir_instruction *out = malloc(sizeof(struct ir_instruction));
    out->op.type = type;
    out->op.one = one;
    out->op.two = two;
    out->op.three = three;
    out->op.is_phi = false;
    out->list = LIST_INIT();
    out->is_label = false;

    return out;
}

struct ir_instruction *ir_inst_alloc_label(size_t label_num)
{
    struct ir_instruction *out = malloc(sizeof(struct ir_instruction));
    out->list = LIST_INIT();
    out->is_label = true;
    out->label.label_num = label_num;
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
    out->is_phi = true;
    return out;
}

void ir_add_instruction(struct ir_program *program, struct ir_instruction *ins)
{
    list_push_end(&ins->list, &program->instructions);
}

struct ir_value *ast_value_convert_ir(struct ir_program *program,
                                      struct ast_value value)
{
    if (value.is_var)
    {
        struct ir_variable *var = ir_variable_lookup(program, value.varname);

        NOT_NULL(var, "undeclared variable %s", value.varname);
        logd("var items:%p", var->values.items);

        /* FIXME: this will make it so that the most recent version is always
         * used before converting to ssa, which is stupid */
        struct ir_variable_version prev_version = var->values.items[0];
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
        ir_value_alloc_label(program, IR_PROGRAM_CURRENT_INS(program));
    struct ir_instruction *label_op =
        ir_inst_alloc_label(label_value->label.label_num);
    list_push_end(&label_op->list, &program->instructions);
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
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_ADD, out, lhs, rhs));
            break;
        case TOKEN_MINUS:
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_SUB, out, lhs, rhs));
            break;
        case TOKEN_STAR:
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_MUL, out, lhs, rhs));
            break;
        case TOKEN_LESS_EQUAL:
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_SET_LE, out, lhs, rhs));
            break;

        /* might not work */
        case TOKEN_ASSIGN:
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_MOV, out, lhs, rhs));
            break;

        case TOKEN_EQUALS:
            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_SET_EQ, out, lhs, rhs));
            break;
        case TOKEN_AND:
            /* this could be done by codegen, and there might be more
             * optimizations that could be done there */

            ir_add_instruction(
                program,
                ir_inst_alloc_op(IR_OP_MOV, out,
                                 ir_value_alloc_immediate(program, 0), NULL));

            struct ir_instruction *jump_op =
                ir_inst_alloc_op(IR_OP_JZ, lhs, NULL, NULL);
            ir_add_instruction(program, jump_op);

            ir_add_instruction(program,
                               ir_inst_alloc_op(IR_OP_SET_NZ, out, rhs, NULL));

            jump_op->op.two = ir_generate_label(program);
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
                                  struct ir_value *dest, struct ast_expr *expr)
{
    if (expr->type == AST_EXPRESSION_VALUE)
    {
        struct ir_value *expr_value =
            ast_value_convert_ir(program, expr->value);
        struct ir_value *dest_value = dest;
        ir_add_instruction(
            program, ir_inst_alloc_op(IR_OP_MOV, dest_value, expr_value, NULL));
        return dest_value;
    }
    else
    {
        struct ir_value *result = ir_generate_expr_impl(program, expr);
        ir_add_instruction(program,
                           ir_inst_alloc_op(IR_OP_MOV, dest, result, NULL));
        return dest;
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

    /* struct ir_variable *var = */
    /*     ir_variable_lookup(program, expr->lhs->value.varname); */

    struct ir_value *lhs = ast_value_convert_ir(program, expr->lhs->value);
    struct ir_value *result = ir_generate_expr(program, lhs, expr->rhs);

    /* struct ir_instruction *result_op = IR_PROGRAM_CURRENT_INS(program); */
    /* struct ir_variable_version version = (struct ir_variable_version){ */
    /* /1* might be bad idea to take addr here *1/ */
    /* &result_op->op, result, program->current_scope_depth}; */
    /* array_macro_append(&var->values, version); */
}

void ir_generate_statement(struct ir_program *program,
                           struct ast_statement *statement)
{

    switch (statement->type)
    {
    case AST_STATEMENT_EXPR:
        ir_generate_expr_assign(program, statement->expr);

        break;
    case AST_STATEMENT_IF:;
        struct ir_value *if_val = ir_value_alloc_reg(program);
        ir_generate_expr(program, if_val, statement->block_expr);

        program->current_scope_depth++;

        struct ir_instruction *jump_op =
            ir_inst_alloc_op(IR_OP_JZ, if_val, NULL, NULL);
        ir_add_instruction(program, jump_op);

        ir_generate_block(program, &statement->block);
        jump_op->op.two = ir_generate_label(program);

        program->current_scope_depth--;

        break;
    case AST_STATEMENT_WHILE:;

        struct ir_value *loop_start = ir_generate_label(program);
        program->current_scope_depth++;

        struct ir_value *condition = ir_value_alloc_reg(program);
        ir_generate_expr(program, condition, statement->block_expr);

        struct ir_instruction *cond_jump_op =
            ir_inst_alloc_op(IR_OP_JZ, condition, NULL, NULL);
        ir_add_instruction(program, cond_jump_op);
        ir_generate_block(program, &statement->block);

        ir_add_instruction(program,
                           ir_inst_alloc_op(IR_OP_JMP, loop_start, NULL, NULL));
        cond_jump_op->op.two = ir_generate_label(program);

        program->current_scope_depth--;

        break;
    case AST_STATEMENT_FOR:
        break;
    case AST_STATEMENT_DECLARE:;
        struct ir_variable *var = ir_variable_alloc(statement->varname);
        array_macro_append(&program->variables, var);
        struct ir_value *val = ir_value_alloc_reg(program);
        ir_generate_expr(program, val, statement->init_value);

        struct ir_variable_version version =
            /* taking address of op here might be stupid */
            (struct ir_variable_version){&IR_PROGRAM_CURRENT_INS(program)->op,
                                         val, program->current_scope_depth};
        array_macro_append(&var->values, version);

        break;
    case AST_STATEMENT_RETURN:;
        /* TODO: there is probably a better way of doing this than
         * allocating a new register, there could be a special return value
         * register that would be automatically mapped to the architecture
         * return value */

        struct ir_value *return_dest = ir_value_alloc_reg(program);
        ir_generate_expr(program, return_dest, statement->expr);
        ir_add_instruction(
            program, ir_inst_alloc_op(IR_OP_RET, return_dest, NULL, NULL));
        break;
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
            log_level(*level, "%d", value->label.label_num);
            log_level(*level, ")");
            log_multiline_end(level);
            break;
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

void ir_instruction_log(struct log_level *level, struct ir_instruction *inst)
{
    if (!inst->is_label)
    {
        ir_operation_log(&inst->op, level);
    }
    else
    {
        log_level(*level, "LABEL %d", inst->label.label_num);
        log_level(*level, "\n");
    }
}

void ir_display(struct ir_program *program, struct log_level *level)
{
    log_multiline_start(level);
    struct list *pos;
    list_for_each(pos, &program->instructions)
    {
        struct ir_instruction *inst =
            containerof(pos, struct ir_instruction, list);
        ir_instruction_log(level, inst);
    }
    log_level(*level, "\n");
    log_multiline_end(level);
}

void ir_generate_block(struct ir_program *program, struct list *statements)
{
    struct list *pos;
    list_for_each(pos, statements)
    {
        ir_generate_statement(program,
                              containerof(pos, struct ast_statement, list));
    }
}

void ir_generate_cfg(struct ir_program *program, struct cfg_node *node)
{
    for (size_t i = 0; i < node->children.num; i++)
    {
        struct cfg_node *node = array_at(&node->children, i);
        if (node->parents.num >= 2)
        {
            for (size_t j = 0; j < node->parents.num; j++)
            {
                struct cfg_node *current = array_at(&node->parents, j);
            }
        }
    }
}

/* TODO: FIXME: the cfg node dominance code is very very very very bad.
 * the time complexity is probably larger than O(n^4), this could be somewhat
 * fixed with an actual set implementation using a hashmap. Also, to detect
 * changing dominance, you could just see if the intersection found anything
 * that was not in the set
 *
 * the array_remove calls also are probably pretty expensive
 */

/* gets immediate dominator of node */
/* this is probably not correct, but i dont care right now */
static struct cfg_node *cfg_get_idom(struct cfg_node *node)
{
    ASSERT(node->dominance.num > 0, "node does not have any dominators set");
    return array_at(&node->dominance, node->dominance.num - 1);
}

void cfg_set_dominance_frontier(struct cfg_parser *parser)
{
    for (size_t i = 0; i < parser->nodes.num; i++)
    {
        struct cfg_node *node = array_at(&parser->nodes, i);
        if (node->parents.num >= 2)
        {
            struct cfg_node *node_idom = cfg_get_idom(node);
            for (size_t j = 0; j < node->parents.num; j++)
            {
                struct cfg_node *parent = array_at(&node->parents, j);
                struct cfg_node *current = parent;
                while (current != node_idom)
                {
                    array_macro_append(&current->dominance_frontier, node);
                    current = cfg_get_idom(current);
                }
            }
        }
    }
}

/* true if removed an item from @dest's dominance */
static bool cfg_node_set_dominance_intersection(struct cfg_parser *parser,
                                                struct cfg_node *dest,
                                                struct cfg_node *other)
{

    bool retval = false;
    for (size_t i = dest->dominance.num; i > 0; i--)
    {
        for (size_t j = 0; j < other->dominance.num; j++)
        {
            if (array_at(&dest->dominance, i) != array_at(&other->dominance, j))
            {
                array_remove(&dest->dominance, i);
                retval = true;
            }
        }
    }
    return retval;
}

static bool cfg_node_set_new_dominance(struct cfg_parser *parser,
                                       struct cfg_node *dest,
                                       struct cfg_node *current)
{
    /* order probably does not matter, but calling this here is nice for
     * debugging */
    bool retval = cfg_node_set_dominance_intersection(parser, dest, current);

    for (size_t i = 0; i < current->parents.num; i++)
    {
        cfg_node_set_new_dominance(parser, dest,
                                   array_at(&current->parents, i));
    }

    return retval;
}

void cfg_set_dominance(struct cfg_parser *parser, struct cfg_node *root)
{
    for (size_t i = 0; i < parser->nodes.num; i++)
    {
        struct cfg_node *node = array_at(&parser->nodes, i);
        array_copy_other(&node->dominance, &parser->nodes);
    }

    bool is_changing;
    do
    {

        is_changing = false;

        for (size_t i = 0; i < parser->nodes.num; i++)
        {
            struct cfg_node *node = array_at(&parser->nodes, i);
            if (cfg_node_set_new_dominance(parser, node, node))
            {
                is_changing = true;
            }
        }
    } while (is_changing);
}

struct cfg_node *cfg_node_alloc()
{
    struct cfg_node *out = malloc(sizeof(struct cfg_node));
    out->children.items = NULL;
    out->children.capacity = 0;
    out->children.num = 0;

    out->instructions.items = NULL;
    out->instructions.capacity = 0;
    out->instructions.num = 0;

    out->parents.items = NULL;
    out->parents.capacity = 0;
    out->parents.num = 0;

    out->dominance.items = NULL;
    out->dominance.capacity = 0;
    out->dominance.num = 0;

    out->dominance_frontier.items = NULL;
    out->dominance_frontier.capacity = 0;
    out->dominance_frontier.num = 0;

    return out;
}

void cfg_node_add_child(struct cfg_parser *parser, struct cfg_node *parent,
                        struct cfg_node *child)
{
    array_macro_append(&parent->children, child);
    array_macro_append(&child->parents, parent);

    /* FIXME: parser.nodes should definitly use a hashmap or something, i just
     * dont want to do it right now */
    for (size_t i = 0; i < parser->nodes.num; i++)
    {
        if (child == array_at(&parser->nodes, i))
        {
            return;
        }
    }
    array_macro_append(&parser->nodes, child);
}

struct cfg_node *cfg_parse_jump(struct cfg_parser *parser,
                                struct ir_program *program,
                                struct ir_instruction *jump_inst,
                                struct cfg_node *dest, int end_label_num)
{
    struct ir_operation *jump = &jump_inst->op;
    struct ir_value *label_value = jump->two;

    struct ir_label *label =
        &array_at(&program->labels, label_value->label.label_num);
    struct cfg_node *child = cfg_parse(
        parser, program, label_value->label.label_num, jump_inst->list.next);
    cfg_node_add_child(parser, dest, child);

    struct cfg_node *block_after = cfg_parse(parser, program, end_label_num,
                                             label->instruction->list.next);

    cfg_node_add_child(parser, child, block_after);
    cfg_node_add_child(parser, dest, block_after);

    return block_after;
}

/* -1 end_label_num for no end label */
struct cfg_node *cfg_parse(struct cfg_parser *parser,
                           struct ir_program *program, int end_label_num,
                           struct list *start_list)
{
    struct cfg_node *out = cfg_node_alloc();

    for (struct list *pos = start_list; pos != (&program->instructions);
         pos = pos->next)
    {
        struct ir_instruction *inst =
            containerof(pos, struct ir_instruction, list);

        /* FIXME: ir_operation is_jump function */
        if (!inst->is_label && inst->op.type == IR_OP_JZ)
        {
            struct cfg_node *ending_block =
                cfg_parse_jump(parser, program, inst, out, end_label_num);
            return out;
        }
        else
        {
            if (inst->is_label && inst->label.label_num == end_label_num)
            {
                return out;
            }
            array_macro_append(&out->instructions, inst);
        }
    }

    return out;
}

void cfg_display(struct cfg_node *node)
{
    logd("cfg block start\n");
    for (size_t i = 0; i < node->instructions.num; i++)
    {
        struct ir_instruction *inst = array_at(&node->instructions, i);
        ir_instruction_log(&LOG_DEBUG, inst);
    }

    for (size_t i = 0; i < node->children.num; i++)
    {
        cfg_display(array_at(&node->children, i));
    }
    logd("cfg block end\n");
}
