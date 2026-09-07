#ifndef C_COMPILER_INTERPRETER_H
#define C_COMPILER_INTERPRETER_H

#include <lib/array.h>
#include <lib/compiler/ast.h>
#include <lib/list.h>
#include <types.h>

/* interpreter part, maybe move this later */

struct interp_type
{
    char *name;
    /* the size of this type, in bytes */
    size_t size;
};

struct scope
{
    struct list list;
    struct array vars;
};

struct interpreter
{
    /* the current scopes the interpreter is in for one function, when a
     * function is called, the scope of that function will be stored on the
     * stack, to not interfere with the scope of the called function  */
    struct list *all_scopes;
    struct list *current_scope;
    struct array funcs; /* this should probably used a linked list, as it is
                           currently storing pointers */
    u8 *memory;
    size_t string_offset; /* TODO: string literals should probably be stored in
                             their own area */
    size_t var_offset;
};

static inline struct interpreter *interpreter_init()
{
    struct interpreter *out = malloc(sizeof(struct interpreter));
    out->funcs = ARRAY_INIT();
    out->all_scopes = malloc(sizeof(struct list));
    list_init_head(out->all_scopes);
    out->current_scope = out->all_scopes;
    out->memory = malloc(1000);
    out->string_offset = 0;
    out->var_offset = 500;
    return out;
}

struct interp_value
{
    struct interp_type *type;
    /* the number of pointers that this value's type has */
    size_t num_pointers;
    u64 value;
};
struct interp_value *statements_eval(struct list *statements_head,
                                     struct interpreter *);
struct interp_value *expr_eval(struct ast_expr *expr, struct interpreter *);
int eval_program(struct list *top_statements_head);

#endif
