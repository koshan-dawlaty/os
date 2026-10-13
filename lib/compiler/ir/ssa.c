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
