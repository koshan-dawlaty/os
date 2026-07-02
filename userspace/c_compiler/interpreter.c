#include "c_compiler.h"
#include <container_of.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <tests/tests.h>
#include <unistd.h>

struct scope *scope_alloc()
{
    struct scope *out = malloc(sizeof(struct scope));

    out->list = LIST_INIT();
    out->vars = ARRAY_INIT();

    return out;
}

struct interp_value *interp_value_alloc(struct ast_type *type, u64 value)
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
struct variable *variable_lookup(struct list *scope_head, const char *search)
{

    struct list *pos;
    list_for_each(pos, scope_head)
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

            if (!(expr->lhs->type == AST_EXPRESSION_VALUE &&
                  expr->lhs->value.is_var) &&
                !(expr->lhs->type == AST_EXPRESSION_UNARY_OPERATOR &&
                  !strcmp(expr->lhs->unary_operator.operator, "*")))
            {
                PANIC("attempt to assign to not variable");
            }

            if (expr->lhs->type == AST_EXPRESSION_UNARY_OPERATOR &&
                !strcmp(expr->lhs->unary_operator.operator, "*"))
            {
                logd("dereference");
                struct interp_value *value =
                    expr_eval(expr->lhs->unary_operator.expr, interp);
                logd("ptr value:%d", value->value);
                *((u8 *)interp->memory + value->value) =
                    expr_eval(expr->rhs, interp)->value;

                return value;
            }
            else if (expr->lhs->type == AST_EXPRESSION_VALUE)
            {
                struct variable *var = variable_lookup(
                    interp->all_scopes, expr->lhs->value.varname);
                NOT_NULL(var, "variable not found:%s",
                         expr->lhs->value.varname);
                var->value = expr_eval(expr->rhs, interp);
                return var->value;
            }
            else
            {
                PANIC("TODO: error");
            }
        }
        struct interp_value *lhs_val = expr_eval(expr->lhs, interp);
        struct interp_value *rhs_val = expr_eval(expr->rhs, interp);

        /* for now, just assume that if there is an operator, both sides are
         * ints */
        /* TODO: maybe add asserts to ensure that they are ints */

        int lhs = lhs_val->value;
        int rhs = rhs_val->value;

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
        else if (!strcmp(expr->operator, "=="))
        {
            result = lhs == rhs;
        }
        else if (!strcmp(expr->operator, "*"))
        {
            result = lhs * rhs;
        }
        else
        {
            PANIC("unknown operator type for expr_eval() %s", expr->operator);
        }

        struct ast_type *type = ast_type_alloc(
            "int", MAX(lhs_val->type->num_pointers,
                       rhs_val->type->num_pointers)); /* TODO: this should
work for pointer math, and should use the largest size of the operands */
        return interp_value_alloc(type, result);
    }
    else if (expr->type == AST_EXPRESSION_UNARY_OPERATOR)
    {
        if (!strcmp(expr->unary_operator.operator, "*"))
        {
            struct interp_value *value =
                expr_eval(expr->unary_operator.expr, interp);

            logd("name:%s", value->type->name);
            if (value->type->num_pointers <= 0)
            {
                PANIC("attempt to dereference not a pointer");
            }

            u8 *ptr = interp->memory + value->value;
            return interp_value_alloc(ast_type_alloc("int", 0), *ptr);
        }
        else
        {
            PANIC("unknown unary operator type for expr_eval() %s",
                  expr->operator);
        }
    }
    else if (expr->type == AST_EXPRESSION_VALUE)
    {

        if (!expr->value.is_var)
        {

            switch (expr->value.literal.type)
            {
            case AST_LITERAL_TYPE_STRING:;
                size_t len = strlen(expr->value.literal.str);

                memcpy(interp->memory + interp->string_offset,
                       expr->value.literal.str, len + 1);
                struct interp_value *out = interp_value_alloc(
                    ast_type_alloc("char", 1), interp->string_offset);
                interp->string_offset += len + 1;

                return out;
            case AST_LITERAL_TYPE_INT:
                return interp_value_alloc(ast_type_alloc("int", 0),
                                          expr->value.literal.integer);
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
            struct list *pos;
            list_for_each(pos, &expr->call.arguments_head)
            {
                struct ast_function_arg *arg =
                    containerof(pos, struct ast_function_arg, list);
                NOT_NULL(arg->expr, "arg->expr was null");

                struct interp_value *val = expr_eval(arg->expr, interp);
                if (!strcmp(val->type->name, "char") &&
                    val->type->num_pointers == 1)
                {
                    u8 *str = interp->memory + val->value;
                    logi("print:%s", str);
                    printf("%s\n", str);
                }
                else if (!strcmp(val->type->name, "int") &&
                         val->type->num_pointers == 0)
                {
                    logi("print:%d", val->value);
                    printf("%d\n", val->value);
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

                struct list *passed_pos = expr->call.arguments_head.next;
                struct list *def_pos;
                list_for_each(def_pos, &func->args_head)
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

                struct list *prev_scope = interp->all_scopes;

                interp->all_scopes = malloc(sizeof(struct list));
                list_init_head(interp->all_scopes); /* clear scope */
                list_push(&new_scope->list, interp->all_scopes);
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

struct interp_value *statements_eval(struct list *statements_head,
                                     struct interpreter *interp)
{
    struct scope *new_scope = scope_alloc();
    list_add_after(&new_scope->list, interp->current_scope);
    interp->current_scope = &new_scope->list;

    struct interp_value *retval = NULL;

    struct list *pos;
    list_for_each(pos, statements_head)
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
                init_val = interp_value_alloc(statement->declared_type, 0);
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
            /* TODO: add error if it is not an int maybe */
            if (value->value != 0)
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
            /* TODO: add error if it is not an int maybe */
            while (value->value != 0)
            {
                struct interp_value *output =
                    statements_eval(&statement->block, interp);
                if (output != NULL)
                {
                    retval = output;
                    goto end;
                }
                value = expr_eval(statement->block_expr, interp);
            }
            break;
        case AST_STATEMENT_FOR:
            expr_eval(statement->for_expr.expr1, interp);
            value = expr_eval(statement->for_expr.expr2, interp);
            /* TODO: add error if it is not an int maybe */
            while (value->value != 0)
            {
                struct interp_value *output =
                    statements_eval(&statement->block, interp);
                if (output != NULL)
                {
                    retval = output;
                    goto end;
                }
                expr_eval(statement->for_expr.expr3, interp);
                value = expr_eval(statement->for_expr.expr2, interp);
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
    list_remove(&new_scope->list);

    return retval;
}

int eval_program(struct list *top_statements_head)
{
    struct interpreter *i = interpreter_init();

    struct list *main_body = NULL;

    struct list *pos;
    list_for_each(pos, top_statements_head)
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
    if (val == NULL)
    {
        return 0;
    }
    return val->value;
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

int main(int argc, char **argv)
{
    init_libc();
    call_constructors();

#ifdef TESTS
    run_tests();
#endif

    LIST_DEFINE_HEAD(tokens);
    const char *program;
    if (argc <= 1)
    {
        // clang-format off
	program = MACRO_STR(

/* int fib(int num) { */
/* 	if (num > 2) { */
/* 		print("in function"); */
/* 		print(num); */
/* 		return fib(num - 1) + fib(num - 2); */
/* 	} */
/* 	return 1; */
/* } */


	
int main() {
print("hello world");
}


		);

        // clang-format on
    }
    else
    {
        int fd = open(argv[1], O_RDONLY);

        size_t read_size = 10;
        size_t buf_len = read_size;

        char *buf = malloc(buf_len);
        size_t buf_offset = 0;
        size_t bytes_read;
        while ((bytes_read = read(fd, buf + buf_offset, read_size)) >=
               read_size)
        {

            if (bytes_read < 0)
            {
                PANIC("bad fd");
            }

            buf_len += read_size;
            buf_offset += read_size;
            buf = realloc(buf, buf_len);
        }
        buf[buf_offset + bytes_read] = 0;

        program = buf;
    }
    tokenizer_parse_tokens(program, &tokens);

    /* ast_parse_tokens(&tokens); */

    LIST_DEFINE_HEAD(top_statements_head);

    struct ast_error *err = ast_parse(&tokens, &top_statements_head);
    if (err != NULL)
    {
        static char buf[100];
        ast_error_to_string(err, buf, 100);
        PANIC("ast error:%s", buf);
    }
    else
    {
        eval_program(&top_statements_head);
    }
    ast_top_statements_free(&top_statements_head);
    logd("done freeing");

    /* statements_eval_simple(&statements_head); */

    exit(0);

    return 0;
}
