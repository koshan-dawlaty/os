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

struct interp_value *interp_value_alloc(struct ast_type *type,
                                        struct ast_value value)
{
    struct interp_value *out = malloc(sizeof(struct interp_value));

    out->value = value;
    out->type = type;

    return out;
}

struct variable
{
    char *varname;
    struct interp_value *value;
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

struct interp_value *expr_eval(struct ast_expr *expr,
                               struct interpreter *interp)
{
    NOT_NULL(expr, "expression was null");
    if (expr->type == AST_EXPRESSION_OPERATOR)
    {

        if (!strcmp(expr->operator, "="))
        {
            if (!expr->lhs->value.is_var)
            {
                PANIC("attempt to assign to not variable");
            }

            struct variable *var =
                variable_lookup(interp->all_scopes, expr->lhs->value.varname);
            NOT_NULL(var, "variable not found:%s", expr->lhs->value.varname);
            var->value = expr_eval(expr->rhs, interp);

            return var->value;
        }
        struct interp_value *lhs_val = expr_eval(expr->lhs, interp);
        struct interp_value *rhs_val = expr_eval(expr->rhs, interp);

        /* for now, just assume that if there is an operator, both sides are
         * ints */

        ASSERT(lhs_val->value.literal.type == AST_LITERAL_TYPE_INT,
               "operator %s needs to be an int", expr->operator);
        ASSERT(rhs_val->value.literal.type == AST_LITERAL_TYPE_INT,
               "operator %s needs to be an int", expr->operator);

        int lhs = lhs_val->value.literal.integer;
        int rhs = rhs_val->value.literal.integer;

        int result;

        if (!strcmp(expr->operator,
                    "+")) /* TODO: maybe want to have enum for operator */
        {
            result = lhs + rhs;
        }
        else if (!strcmp(expr->operator, "-"))
        {
            result = lhs - rhs;
        }
        else if (!strcmp(expr->operator, "<"))
        {
            result = lhs < rhs;
        }
        else if (!strcmp(expr->operator, ">"))
        {
            result = lhs > rhs;
        }
        else
        {
            PANIC("unknown operator type for expr_eval() %s", expr->operator);
        }

        struct ast_type *type = ast_type_alloc(
            "int", 0); /* TODO: this should work for pointer math, and should
                          use the largest size of the operands */
        return interp_value_alloc(type,
                                  AST_VALUE_LITERAL(AST_LITERAL_INT(result)));
    }
    else if (expr->type == AST_EXPRESSION_VALUE)
    {

        if (!expr->value.is_var)
        {

            switch (expr->value.literal.type)
            {
                /* TODO: bad */
            case AST_LITERAL_TYPE_STRING:
                return interp_value_alloc(ast_type_alloc("char", 1),
                                          expr->value);
            case AST_LITERAL_TYPE_INT:
                return interp_value_alloc(ast_type_alloc("int", 0),
                                          expr->value);
                break;
            }
        }
        else
        {
            struct variable *var =
                variable_lookup(interp->all_scopes, expr->value.varname);

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

                struct interp_value *val = expr_eval(arg->expr, interp);
                if (!strcmp(val->type->name, "char") &&
                    val->type->num_pointers == 1)
                {
                    logi("print:%s", val->value.literal.str);
                }
                else if (!strcmp(val->type->name, "int") &&
                         val->type->num_pointers == 0)
                {
                    logi("print:%d", val->value.literal.integer);
                }
                else
                {
                    PANIC("dont know how to print %s (%d pointers)",
                          val->type->name, val->type->num_pointers);
                }
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

                    struct interp_value *value =
                        expr_eval(passed_arg->expr, interp);
                    struct variable var = {.value = value,
                                           .varname = def_arg->name};
                    array_add(&new_scope->vars, &var, sizeof(struct variable));
                    passed_pos = passed_pos->next;
                }

                struct list2c *prev_scope = interp->all_scopes;

                interp->all_scopes = malloc(sizeof(struct list2c));
                list2c_init_head(interp->all_scopes); /* clear scope */
                list2c_push(&new_scope->list, interp->all_scopes);
                interp->current_scope = &new_scope->list;

                if (passed_pos != &expr->call.arguments_head)
                {
                    PANIC("passed more arguments than required");
                }

                struct interp_value *return_value =
                    statements_eval(&func->block, interp);
                interp->all_scopes = prev_scope;
                interp->current_scope = prev_scope->next;

                return return_value;
            }
        }

        PANIC("unknown function name:%s", expr->call.function_name);
    }
    UNREACHABLE("unknown enum type:%d", expr->type);
}

struct interp_value *statements_eval(struct list2c *statements_head,
                                     struct interpreter *interp)
{
    struct scope *new_scope = scope_alloc();
    list2c_add_after(&new_scope->list, interp->current_scope);
    interp->current_scope = &new_scope->list;

    struct interp_value *retval = NULL;

    struct list2c *pos;
    list2c_for_each(pos, statements_head)
    {

        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);

        struct interp_value *value;
        switch (statement->type)
        {
        case AST_STATEMENT_DECLARE:;
            struct interp_value *init_val = NULL;
            if (statement->init_value != NULL)
            {
                init_val = expr_eval(statement->init_value, interp);
            }
            else
            {
                init_val = interp_value_alloc(statement->declared_type,
                                              AST_VALUE_LITERAL(0));
            }
            struct variable var = {statement->varname, init_val};
            NOT_NULL(interp->current_scope, "current scope was null");
            struct scope *current =
                containerof(interp->current_scope, struct scope, list);
            array_push(&current->vars, &var, sizeof(struct variable));
            variable_lookup(interp->all_scopes, "retval");

            break;
        case AST_STATEMENT_EXPR:
            expr_eval(statement->expr, interp);
            break;
        case AST_STATEMENT_IF:;
            value = expr_eval(statement->block_expr, interp);
            if (value->value.literal.type != AST_LITERAL_TYPE_INT)
            {
                PANIC("expected int for if condition");
            }
            if (value->value.literal.integer != 0)
            {
                struct interp_value *output =
                    statements_eval(&statement->block, interp);
                if (output != NULL)
                {
                    retval = output;
                    goto end;
                }
            }
            break;
        case AST_STATEMENT_WHILE:;
            value = expr_eval(statement->block_expr, interp);
            if (value->value.literal.type != AST_LITERAL_TYPE_INT)
            {
                PANIC("expected int for while loop condition");
            }
            while (value->value.literal.integer != 0)
            {
                struct interp_value *output =
                    statements_eval(&statement->block, interp);
                if (output != NULL)
                {
                    retval = output;
                    goto end;
                }
                value = expr_eval(statement->block_expr, interp);
                if (value->value.literal.type != AST_LITERAL_TYPE_INT)
                {
                    PANIC("expected int for while loop condition");
                }
            }
            break;
        case AST_STATEMENT_RETURN:
            if (statement->expr != NULL)
            {
                retval = expr_eval(statement->expr, interp);
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

    struct interp_value *val = statements_eval(main_body, i);
    return val->value.literal.integer;
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

/* int fib(int num) { */
/* 	if (num > 2) { */
/* 		print("in function"); */
/* 		print(num); */
/* 		return fib(num - 1) + fib(num - 2); */
/* 	} */
/* 	return 1; */
/* } */

int func(int num) {
	return num + 1;
}

int main() {
	func(10);
	int result = 10;
	print(result);
}

		);

    // clang-format on
    tokenizer_parse_tokens(program, &tokens);

    /* ast_parse_tokens(&tokens); */

    LIST2C_DEFINE_HEAD(top_statements_head);

    ast_parse(&tokens, &top_statements_head);
    eval_program(&top_statements_head);
    /* statements_eval_simple(&statements_head); */

#ifdef TESTS
    run_tests();
#endif

    exit(0);

    return 0;
}
