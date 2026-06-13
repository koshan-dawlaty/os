#include "c_compiler.h"
#include "container_of.h"
#include "lib/list.h"
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <tests/tests.h>

struct variable
{
    char *varname;
    int value;
};

/* very simple test function for evaluating a simple expression */
int expr_eval(struct ast_expr *expr, struct interpreter *interp)
{
    if (expr->type == AST_EXPRESSION_OPERATOR)
    {
        if (!strcmp(expr->operator,
                    "+")) /* TODO: maybe want to have enum for operator */
        {
            int lhs = expr_eval(expr->lhs, interp);
            int rhs = expr_eval(expr->rhs, interp);
            return lhs + rhs;
        }
        else if (!strcmp(expr->operator, "="))
        {
            if (!expr->lhs->value.is_var)
            {
                PANIC("attempt to assign to not variable");
            }

            for (size_t i = 0;
                 i < ARRAY_LEN(interp->vars, sizeof(struct variable)); i++)
            {
                struct variable *var =
                    array_get(&interp->vars, i, sizeof(struct variable));

                if (!strcmp(expr->lhs->value.varname, var->varname))
                {
                    var->value = expr_eval(expr->rhs, interp);
                    return var->value;
                }
            }
            PANIC("variable not found:%s", expr->lhs->value.varname);
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
            for (size_t i = 0;
                 i < ARRAY_LEN(interp->vars, sizeof(struct variable)); i++)
            {
                struct variable *var =
                    array_get(&interp->vars, i, sizeof(struct variable));

                if (!strcmp(expr->value.varname, var->varname))
                {
                    return var->value;
                }
            }
            PANIC("variable not found:%s", expr->value.varname);
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

                /* FIXME: clear all of the non-global variables here */

                statements_eval(&func->block, interp);

                return 0; /* TODO: change this when there is a better type
                         system for the interpreter */
            }
        }

        PANIC("unknown function name:%s", expr->call.function_name);
    }
    UNREACHABLE("unknown enum type:%d", expr->type);
}

void statements_eval(struct list2c *statements_head, struct interpreter *interp)
{
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
            array_push(&interp->vars, &var, sizeof(struct variable));
            break;
        case AST_STATEMENT_EXPR:
            expr_eval(statement->expr, interp);
            break;
        case AST_STATEMENT_IF:;
            value = expr_eval(statement->block_expr, interp);
            if (value != 0)
            {
                statements_eval(&statement->block, interp);
            }
            break;
        case AST_STATEMENT_WHILE:;
            value = expr_eval(statement->block_expr, interp);
            while (value != 0)
            {
                statements_eval(&statement->block, interp);
                value = expr_eval(statement->block_expr, interp);
            }
            break;

        default:
            UNREACHABLE("unhandled switch case:%d", statement->type);
        }
    }
}

void eval_program(struct list2c *top_statements_head)
{
    struct interpreter i = INTERPRETER_INIT();

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

        array_add(&i.funcs, &statement->function,
                  sizeof(struct ast_function_declare *));
    }

    NOT_NULL(main_body, "main() not found!");

    statements_eval(main_body, &i);
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
    tokenizer_parse_tokens(
	/* "int z;" */
	/* "z = 10;" */

	"void f() {"
	"print(123);"
	"print(456);"
	"int z = 0;"
	"while (z < 9) {"
	" print(z);"
	" z = z + 1;"
	"}"
	"}"
	
    "void main() {"
	"f();"
    "}"

    , &tokens);

    // clang-format on
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
