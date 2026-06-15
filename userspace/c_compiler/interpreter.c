#include "c_compiler.h"
#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <math.h>
#include <string.h>
#include <tests/tests.h>

struct scope *scope_alloc()
{
    struct scope *out = malloc(sizeof(struct scope));

    out->list = LIST2C_INIT();
    out->vars = ARRAY_INIT();

    return out;
}

struct variable
{
    char *varname;
    int value;
};

/* TODO: this won't work correctly if something is declared in an inner scope
 * with the same name as something in an outer scope, it might work to just
 * traverse the list backwards to fix that though */
/*
 * returns the variable with name matching @search from all of the scopes in
 * @scope_head
 *
 * returns null if nothing was found
 *
 * you dont own the returned pointer, it is from the array in the
 * scope
 */
struct variable *variable_lookup(struct list2c *scope_head, const char *search)
{
    struct list2c *pos;
    list2c_for_each(pos, scope_head)
    {
        struct scope *s = containerof(pos, struct scope, list);
        for (size_t i = 0; i < ARRAY_LEN(s->vars, sizeof(struct variable)); i++)
        {
            struct variable *var =
                array_get(&s->vars, i, sizeof(struct variable));
            NOT_NULL(var, "variable from array was null");
            NOT_NULL(var->varname, "variable had null name");

            if (!strcmp(var->varname, search))
            {
                return var;
            }
        }
    }

    return NULL;
}

int expr_eval(struct ast_expr *expr, struct interpreter *interp)
{
    NOT_NULL(expr, "expression was null");
    if (expr->type == AST_EXPRESSION_OPERATOR)
    {
        if (!strcmp(expr->operator,
                    "+")) /* TODO: maybe want to have enum for operator */
        {
            int lhs = expr_eval(expr->lhs, interp);
            int rhs = expr_eval(expr->rhs, interp);
            return lhs + rhs;
        }
        else if (!strcmp(expr->operator, "-"))
        {
            int lhs = expr_eval(expr->lhs, interp);
            int rhs = expr_eval(expr->rhs, interp);
            return lhs - rhs;
        }
        else if (!strcmp(expr->operator, "="))
        {
            if (!expr->lhs->value.is_var)
            {
                PANIC("attempt to assign to not variable");
            }

            struct variable *var =
                variable_lookup(&interp->all_scopes, expr->lhs->value.varname);
            NOT_NULL(var, "variable not found:%s", expr->lhs->value.varname);
            var->value = expr_eval(expr->rhs, interp);
            return var->value;
        }
        else if (!strcmp(expr->operator, "<"))
        {
            int lhs = expr_eval(expr->lhs, interp);
            int rhs = expr_eval(expr->rhs, interp);
            return lhs < rhs;
        }
        else if (!strcmp(expr->operator, ">"))
        {
            int lhs = expr_eval(expr->lhs, interp);
            int rhs = expr_eval(expr->rhs, interp);
            return lhs > rhs;
        }
        else
        {
            PANIC("unknown operator type for expr_eval() %s", expr->operator);
        }
    }
    else if (expr->type == AST_EXPRESSION_VALUE)
    {
        if (!expr->value.is_var)
        {

            return expr->value.literal;
        }
        else
        {
            struct variable *var =
                variable_lookup(&interp->all_scopes, expr->value.varname);
            NOT_NULL(var, "variable not found:%s", expr->value.varname);
            return var->value;
        }
    }
    else if (expr->type == AST_EXPRESSION_CALL)
    {

        if (!strcmp(expr->call.function_name, "print"))
        {
            struct list2c *pos;
            list2c_for_each(pos, &expr->call.arguments_head)
            {
                struct ast_function_arg *arg =
                    containerof(pos, struct ast_function_arg, list);
                NOT_NULL(arg->expr, "arg->expr was null");

                int val = expr_eval(arg->expr, interp);
                logi("print:%d", val);
            }
            return 0;
        }

        for (size_t i = 0; i < ARRAY_LEN(interp->funcs,
                                         sizeof(struct ast_function_declare *));
             i++)
        {
            struct ast_function_declare *func =
                *(struct ast_function_declare **)array_get(
                    &interp->funcs, i, sizeof(struct ast_function_declare *));

            if (!strcmp(func->name, expr->call.function_name))
            {

                struct scope *new_scope = scope_alloc();

                struct list2c *passed_pos = expr->call.arguments_head.next;
                struct list2c *def_pos;
                list2c_for_each(def_pos, &func->args_head)
                {
                    if (passed_pos == &expr->call.arguments_head)
                    {
                        PANIC("passed less arguments than required");
                    }
                    struct ast_function_arg_def *def_arg =
                        containerof(def_pos, struct ast_function_arg_def, list);
                    struct ast_function_arg *passed_arg =
                        containerof(passed_pos, struct ast_function_arg, list);

                    logi("adding:%s to scope", def_arg->name);

                    int value = expr_eval(passed_arg->expr, interp);
                    struct variable var = {.value = value,
                                           .varname = def_arg->name};
                    array_add(&new_scope->vars, &var, sizeof(struct variable));

                    passed_pos = passed_pos->next;
                }

                struct list2c prev_scope = interp->all_scopes;
                list2c_init_head(&interp->all_scopes); /* clear scope */
                list2c_push(&new_scope->list, &interp->all_scopes);
                interp->current_scope = &new_scope->list;

                if (passed_pos != &expr->call.arguments_head)
                {
                    PANIC("passed more arguments than required");
                }

                int return_value = statements_eval(&func->block, interp);
                interp->all_scopes = prev_scope;

                return return_value; /* TODO: change this when there is a better
                         type system for the interpreter */
            }
        }

        PANIC("unknown function name:%s", expr->call.function_name);
    }
    UNREACHABLE("unknown enum type:%d", expr->type);
}

int statements_eval(struct list2c *statements_head, struct interpreter *interp)
{
    struct scope *new_scope = scope_alloc();
    list2c_add_after(&new_scope->list, interp->current_scope);
    interp->current_scope = &new_scope->list;

    int retval = -1;

    struct list2c *pos;
    list2c_for_each(pos, statements_head)
    {

        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);

        int value;
        switch (statement->type)
        {
        case AST_STATEMENT_DECLARE:;
            logd("declaration of %s", statement->varname);
            int init_val = 0;
            if (statement->init_value != NULL)
            {
                init_val = expr_eval(statement->init_value, interp);
            }
            struct variable var = {statement->varname, init_val};
            NOT_NULL(interp->current_scope, "");
            struct scope *current =
                containerof(interp->current_scope, struct scope, list);
            array_push(&current->vars, &var, sizeof(struct variable));
            break;
        case AST_STATEMENT_EXPR:
            expr_eval(statement->expr, interp);
            break;
        case AST_STATEMENT_IF:;
            value = expr_eval(statement->block_expr, interp);
            if (value != 0)
            {
                int output = statements_eval(&statement->block, interp);
                if (output != -1)
                {
                    retval = output;
                    goto end;
                }
            }
            break;
        case AST_STATEMENT_WHILE:;
            value = expr_eval(statement->block_expr, interp);
            while (value != 0)
            {
                int output = statements_eval(&statement->block, interp);
                if (output != -1)
                {
                    retval = output;
                    goto end;
                }
                value = expr_eval(statement->block_expr, interp);
            }
            break;
        case AST_STATEMENT_RETURN:
            if (statement->expr != NULL)
            {
                retval = expr_eval(statement->expr, interp);
                logd("returning %d", retval);
            }
            goto end;

        default:
            UNREACHABLE("unhandled switch case:%d", statement->type);
        }
    }
end:

    interp->current_scope = new_scope->list.prev;
    list2c_remove(&new_scope->list);

    return retval;
}

int eval_program(struct list2c *top_statements_head)
{
    struct interpreter *i = interpreter_init();

    struct list2c *main_body = NULL;

    struct list2c *pos;
    list2c_for_each(pos, top_statements_head)
    {
        struct ast_top_statement *statement =
            containerof(pos, struct ast_top_statement, list);

        ASSERT(statement->type == AST_TOP_STATEMENT_FUNC,
               "only top level functions are implemented in the interpreter");

        if (!strcmp(statement->function->name, "main"))
        /* if (!strcmp(statement->function->name, "f")) */
        {
            main_body = &statement->function->block;
        }

        array_add(&i->funcs, &statement->function,
                  sizeof(struct ast_function_declare *));
    }

    NOT_NULL(main_body, "main() not found!");

    return statements_eval(main_body, i);
}

/* TODO: put in lib/ or make gcc do it in _start */
extern void (*__ctors_start[])(void);
extern void (*__ctors_end[])(void);
void call_constructors(void)
{
    for (void (**ctor)(void) = __ctors_start; ctor < __ctors_end; ++ctor)
    {
        (*ctor)();
    }
}

int main()
{
    init_libc();
    call_constructors();

    LIST2C_DEFINE_HEAD(tokens);
    // clang-format off
	const char * program = MACRO_STR(

int main() {
	int i = 0;
	while (i < 10) {
		i = i + 1;
	}
	return i;
}
		);
    // clang-format on
    tokenizer_parse_tokens(program, &tokens);

    /* ast_parse_tokens(&tokens); */

    LIST2C_DEFINE_HEAD(top_statements_head);

    ast_parse(&tokens, &top_statements_head);
    eval_program(&top_statements_head);
    logi("done");
    /* statements_eval_simple(&statements_head); */

#ifdef TESTS
    run_tests();
#endif

    exit(0);

    return 0;
}
