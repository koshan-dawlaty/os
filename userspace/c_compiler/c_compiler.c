#include "c_compiler.h"
#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/tests.h>

bool token_is_operator(enum token_type type)
{
    return type == TOKEN_ASSIGN || type == TOKEN_PLUS ||
           type == TOKEN_GREATER || type == TOKEN_LESS;
}

const char *token_type_str(enum token_type type)
{
    switch (type)
    {
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_WHILE:
        return "WHILE STATEMENT";
    case TOKEN_IF:
        return "IF STATEMENT";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_UNKNOWN:
        return "UNKNOWN";
    case TOKEN_ASSIGN:
        return "ASSIGN";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_EQUALS:
        return "EQUALS";
    case TOKEN_INT:
        return "INT";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_O_PAREN:
        return "OPEN PARENTHESES";
    case TOKEN_C_PAREN:
        return "CLOSE PARENTHESES";
    case TOKEN_O_BRACE:
        return "OPEN BRACE";
    case TOKEN_C_BRACE:
        return "CLOSE BRACE";
    case TOKEN_GREATER:
        return "GREATER";
    case TOKEN_LESS:
        return "LESS";
    }
}

struct token *token_alloc_init(enum token_type type, char *str, size_t len)
{
    struct token *out = malloc(sizeof(struct token));

    out->token = str;
    out->len = len;
    out->type = type;
    out->list = LIST2C_INIT();

    return out;
}

struct token *token_alloc(enum token_type type, const char *token, size_t len)
{
    return token_alloc_init(type, strndup(token, len), len);
}

/* a version of token_alloc intended to be used on string literals */
struct token *token_alloc_literal(enum token_type type, const char *literal,
                                  size_t *len_ptr)
{
    size_t len = strlen(literal);
    *len_ptr += len;

    return token_alloc_init(type, strdup(literal), len);
}

bool is_space(char c)
{
    return c == ' ' || c == '\n' || c == '\t';
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alpha(char c)
{
    return c >= 'a' && c <= 'z';
}

char *get_next_word(const char *s)
{
    for (; *s != 0; s++)
    {
        if (is_space(*s))
        {
            return (char *)s;
        }
    }
    return (char *)s;
}

struct token_simple_map_entry
{
    const char *s;
    enum token_type type;
};

struct token_simple_map_entry token_simple_map[] = {
    // clang-format off

    {"(", TOKEN_O_PAREN},
    {")", TOKEN_C_PAREN},
    {"{", TOKEN_O_BRACE},
    {"}", TOKEN_C_BRACE},
    {";", TOKEN_SEMICOLON},
    {"+", TOKEN_PLUS},
    {">", TOKEN_GREATER},
    {"<", TOKEN_LESS},
    {"if", TOKEN_IF},
    {"while", TOKEN_WHILE},
    {",", TOKEN_COMMA},

    // clang-format on

};

/*
 * gets a one character token
 */
struct token *get_token_simple(const char *s, size_t *len)
{

    for (size_t i = 0;
         i < (sizeof token_simple_map / sizeof token_simple_map[0]); i++)
    {
        if (!strncmp(s, token_simple_map[i].s, strlen(token_simple_map[i].s)))
        {

            return token_alloc_literal(token_simple_map[i].type,
                                       token_simple_map[i].s, len);
        }
    }

    return NULL;
}

struct token *get_token(const char *input, size_t *len)
{

    while (is_space(input[*len]))
    {
        (*len)++;
    }

    const char *s = input + *len;
    /* logd("|%s|", s); */

    if (*s == 0)
    {
        return token_alloc(TOKEN_EOF, "", 0);
    }

    struct token *simple = get_token_simple(s, len);
    if (simple != NULL)
    {
        return simple;
    }

    if (is_alpha(*s) || *s == '_')
    {
        size_t i = 0;
        while ((is_digit(s[i]) || is_alpha(s[i]) || s[i] == '_') && s[i] != 0)
        {
            i++;
        }
        *len += i;
        return token_alloc(TOKEN_IDENTIFIER, s, i);
    }

    if (is_digit(*s))
    {
        size_t i = 0;
        while (is_digit(s[i]))
        {
            i++;
        }
        *len += i;
        return token_alloc(TOKEN_INT, s, i);
    }

    /* stupid */
    if (s[0] == '=')
    {
        if (s[1] == '=' && s[2] == 0)
        {
            return token_alloc_literal(TOKEN_EQUALS, "==", len);
        }
        else // if (s[1] == 0)
        {
            return token_alloc_literal(TOKEN_ASSIGN, "=", len);
        }
    }

    return token_alloc(TOKEN_UNKNOWN, "", 0);
}

void tokenizer_parse_tokens(const char *program, struct list2c *token_list)
{

    size_t offset = 0;

    size_t len = 0;
    struct token *t = get_token(program, &len);
    offset += len;
    while (t->type != TOKEN_UNKNOWN)
    {

        list2c_push_end(&t->list, token_list);
        if (t->type == TOKEN_EOF)
        {
            return; /* done */
        }

        size_t len = 0;
        t = get_token(program + offset, &len);
        offset += len;
    }
}

struct ast_type *ast_type_alloc(const char *name)
{
    struct ast_type *out = malloc(sizeof(struct ast_type));

    out->name = strdup(name);

    return out;
}

#define AST_VALUE_VAR(_var)                                                    \
    ((struct ast_value){.is_var = true, .varname = _var})
#define AST_VALUE_LITERAL(_literal)                                            \
    ((struct ast_value){.is_var = false, .literal = _literal})

static inline struct ast_value ast_token_to_value(struct token *tok)
{
    if (tok->type == TOKEN_INT)
    {
        int num = atoi(tok->token);
        return AST_VALUE_LITERAL(num);
    }
    else if (tok->type == TOKEN_IDENTIFIER)
    {
        return AST_VALUE_VAR(strdup(tok->token));
    }
    else
    {
        PANIC("error");
    }
}

struct ast_function_arg *ast_function_arg_alloc(struct ast_expr *expr)
{
    struct ast_function_arg *arg = malloc(sizeof(struct ast_function_arg));
    arg->expr = expr;
    arg->list = LIST2C_INIT();

    return arg;
}

struct ast_expr *ast_expr_alloc_call(const char *function_name)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));
    expr->type = AST_EXPRESSION_CALL;
    expr->call =
        (struct ast_function_call){.function_name = strdup(function_name)};
    list2c_init_head(&expr->call.arguments_head);

    return expr;
}

struct ast_expr *ast_expr_alloc_operator(struct ast_expr *lhs,
                                         struct ast_expr *rhs,
                                         const char *operator)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));

    expr->type = AST_EXPRESSION_OPERATOR;
    expr->operator = strdup(operator);
    expr->lhs = lhs;
    expr->rhs = rhs;

    return expr;
}

struct ast_expr *ast_expr_alloc_value(struct ast_value value)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));
    expr->value = value;
    expr->type = AST_EXPRESSION_VALUE;

    return expr;
}

struct ast_statement *ast_statement_alloc_expr(struct ast_expr *expr)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    out->expr = expr;
    out->type = AST_STATEMENT_EXPR;
    out->list = LIST2C_INIT();

    return out;
}

struct ast_statement *
ast_statement_alloc_declaration(struct ast_type *type, const char *varname,
                                struct ast_expr *init_value)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    out->declared_type = type;
    out->type = AST_STATEMENT_DECLARE;
    out->init_value = init_value;
    out->varname = strdup(varname);
    out->list = LIST2C_INIT();

    return out;
}

struct ast_statement *ast_statement_alloc_block(enum ast_statement_type type,
                                                struct ast_expr *block_expr)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    list2c_init_head(&out->block);
    out->type = type;
    out->block_expr = block_expr;
    out->list = LIST2C_INIT();

    return out;
}

struct token *parser_advance(struct ast_parser *parser)
{
    struct token *out = parser->current;
    parser->current = TOKEN_LIST_GET(parser->current->list.next);
    return out;
}

struct token *parser_get_next(struct ast_parser *parser)
{
    return TOKEN_LIST_GET(parser->current->list.next);
}

struct token *parser_get_current(struct ast_parser *parser)
{
    return TOKEN_LIST_GET(&parser->current->list);
}

bool parser_next_is_type(struct ast_parser *parser)
{
    return (parser_get_current(parser)->type ==
            TOKEN_IDENTIFIER); /* TODO: wont work with unsigned or restrict or
                                   pointers */
}

struct ast_type *parser_parse_type(struct ast_parser *parser)
{
    ASSERT(parser_next_is_type(parser), "next token %s (%s) is not a type",
           parser_get_current(parser)->token,
           token_type_str(parser_get_current(parser)->type));

    return ast_type_alloc(parser_advance(parser)->token);
}

struct ast_expr *ast_parse_expression(struct ast_parser *parser);

/* ignores operators, utiltiy */
static struct ast_expr *ast_parse_expression_simple(struct ast_parser *parser)
{

    struct token *tok = parser_advance(parser);

    switch (tok->type)
    {
    case TOKEN_IDENTIFIER:
        if (parser_get_current(parser)->type ==
            TOKEN_O_PAREN) /* function call */
        {

            parser_advance(parser);
            struct ast_expr *out = ast_expr_alloc_call(tok->token);
            struct ast_function_call *call = &out->call;

            if (parser_get_current(parser)->type == TOKEN_C_PAREN)
            {
                parser_advance(parser);
                return out;
            }

            while (true)
            {
                struct ast_expr *expr = ast_parse_expression(parser);
                struct ast_function_arg *arg = ast_function_arg_alloc(expr);
                list2c_push_end(&arg->list, &call->arguments_head);

                struct token *tok = parser_advance(parser);
                /* logd(", or ) %s", tok->token); */

                if (tok->type == TOKEN_C_PAREN)
                {
                    break;
                }
                else if (tok->type != TOKEN_COMMA)
                {
                    PANIC("expected ',' or ')' got %s of type %s", tok->token,
                          token_type_str(tok->type));
                }
            }
            /* parser_advance(parser); */
            return out;
        }
    case TOKEN_INT:
        return ast_expr_alloc_value(ast_token_to_value(tok));

    case TOKEN_O_PAREN:;
        struct ast_expr *out = ast_parse_expression(parser);
        struct token *close_paren = parser_advance(parser);
        if (close_paren->type != TOKEN_C_PAREN)
        {
            PANIC("expected ')' got %s", token_type_str(close_paren->type));
        }
        return out;

    default:
        /* TODO: if it is a keyword, error */
        PANIC("invalid token type:%s for token:%s when parsing expression",
              token_type_str(tok->type), tok->token);
    }
}

struct ast_expr *ast_parse_expression(struct ast_parser *parser)
{
    /* TODO:!!! order of operations */

    struct ast_expr *lhs = ast_parse_expression_simple(
        parser); /* the left hand side should always be an expression that does
                    not require recursion */

    struct token *operator_token = parser_get_current(parser);
    if (token_is_operator(operator_token->type))
    {
        parser_advance(parser);
        struct ast_expr *rhs = ast_parse_expression(parser);

        struct ast_expr *operator =
            ast_expr_alloc_operator(lhs, rhs, operator_token->token);
        return operator;
    }
    else
    {
        return lhs; /* there was no right hand side, so we need to return this
                       to the caller */
    }
}

void ast_parse_statements(struct ast_parser *parser,
                          struct list2c *statements_head);

struct ast_statement *ast_parse_statement(struct ast_parser *parser)
{
    struct token *tok = parser_get_current(parser);

    struct ast_statement *out = NULL;
    /* logd("%s, %s", tok->token, token_type_str(tok->type)); */

    if (tok->type == TOKEN_IDENTIFIER ||
        tok->type == TOKEN_INT) /* TODO: this might not work with
         other kinds of statements */
    {
        struct token *next = parser_get_next(parser);
        /* TODO: wont work with unsigned types, as it checks that next->type is
         * a variable */
        if (parser_next_is_type(parser) &&
            next->type == TOKEN_IDENTIFIER) /* 2 identifiers in a row means it
         must be a declaration */
        {
            struct ast_type *type = parser_parse_type(parser);
            const char *name = next->token;
            struct ast_expr *init_val = NULL;
            parser_advance(parser);
            if (parser_get_current(parser)->type == TOKEN_ASSIGN)
            {
                parser_advance(parser);
                init_val = ast_parse_expression(parser);
            }
            out = ast_statement_alloc_declaration(type, name, init_val);
        }
        else if (next->type != TOKEN_IDENTIFIER)
        {
            out = ast_statement_alloc_expr(ast_parse_expression(parser));
        }
        else
        {
            PANIC("error");
        }

        struct token *semicolon = parser_advance(parser);
        if (semicolon->type != TOKEN_SEMICOLON)
        {
            PANIC("expected semicolon got:%s for token %s",
                  token_type_str(semicolon->type), semicolon->token);
        }
    }
    else if (tok->type == TOKEN_IF)
    {
        parser_advance(parser);
        out = ast_statement_alloc_block(
            AST_STATEMENT_IF,
            ast_parse_expression(
                parser) /* TODO: this will allow things like if x == y {} */);

        ast_parse_statements(parser, &out->block);
    }
    else if (tok->type == TOKEN_WHILE)
    {
        parser_advance(parser);
        out = ast_statement_alloc_block(
            AST_STATEMENT_WHILE,
            ast_parse_expression(
                parser) /* TODO: this will allow things like if x == y {} */);

        ast_parse_statements(parser, &out->block);
    }

    return out;
}

void ast_parse_statements(struct ast_parser *parser,
                          struct list2c *statements_head)
{

    struct token *open_brace = parser_advance(parser);
    if (open_brace->type != TOKEN_O_BRACE)
    {
        PANIC("expected '{' at the start of a block");
    }

    while (parser_get_current(parser)->type != TOKEN_C_BRACE)
    {
        struct ast_statement *statement = ast_parse_statement(parser);
        list2c_push_end(&statement->list, statements_head);
    }
    parser_advance(parser);
}

/* function argument definition */
struct ast_function_arg_def
{
    struct ast_type *type;
    char *name;
    struct list2c list;
};

struct ast_function_arg_def *ast_function_arg_def_alloc(struct ast_type *type,
                                                        const char *name)
{
    struct ast_function_arg_def *out =
        malloc(sizeof(struct ast_function_arg_def));

    out->type = type;
    out->name = strdup(name);
    out->list = LIST2C_INIT();

    return out;
}

struct ast_function_declare
{
    struct ast_type *return_type;
    char *name;
    struct list2c args_head;

    struct list2c block;
};

struct ast_function_declare *
ast_function_declare_alloc(struct ast_type *return_type, const char *name)
{
    struct ast_function_declare *out =
        malloc(sizeof(struct ast_function_declare));
    out->name = strdup(name);
    list2c_init_head(&out->args_head);
    list2c_init_head(&out->block);
    out->return_type = return_type;

    return out;
}

/* top level statements, like type / global variable definitions, and
 * functions */
struct ast_top_statement
{
    enum ast_top_statement_type
    {
        AST_TOP_STATEMENT_FUNC,
        AST_TOP_STATEMENT_VAR,
        AST_TOP_STATEMENT_TYPE,
    } type;

    union
    {
        struct ast_function_declare *function;
    };

    struct list2c list;
};

struct ast_top_statement *ast_parse_top_statement(struct ast_parser *parser)
{
    struct ast_top_statement *out = malloc(sizeof(struct ast_top_statement));
    out->list = LIST2C_INIT();
    struct ast_type *type = parser_parse_type(
        parser); /* will not work for parsing type definitions */

    struct token *name = parser_advance(parser);
    if (name->type != TOKEN_IDENTIFIER)
    {
        PANIC("expected identifier, got %s (%s)", name->token,
              token_type_str(name->type));
    }

    struct token *tok = parser_get_current(parser);

    switch (tok->type)
    {

    case TOKEN_O_PAREN:;
        parser_advance(parser);
        struct ast_function_declare *func =
            ast_function_declare_alloc(type, name->token);

        if (parser_get_current(parser)->type == TOKEN_O_PAREN)
        {
            goto args_done;
        }

        while (true)
        {
            struct ast_type *type = parser_parse_type(parser);
            struct token *name = parser_advance(parser);
            if (name->type != TOKEN_IDENTIFIER)
            {
                PANIC("exepcted identifier, got %s (%s)", name->token,
                      token_type_str(name->type));
            }

            struct token *current = parser_get_current(parser);
            if (current->type == TOKEN_C_PAREN)
            {
                break;
            }
            else if (current->type == TOKEN_COMMA)
            {
                parser_advance(parser);
            }
            else
            {
                PANIC("expected ',' or ')', got %s (%s)", current->token,
                      token_type_str(current->type));
            }

            struct ast_function_arg_def *arg =
                ast_function_arg_def_alloc(type, name->token);
            list2c_push_end(&arg->list, &func->args_head);
        }
    args_done:
        parser_advance(parser); /* ')' */

        ast_parse_statements(parser, &func->block);

        out->function = func;
        out->type = AST_TOP_STATEMENT_FUNC;

        break;
    case TOKEN_SEMICOLON:
    case TOKEN_ASSIGN:
        PANIC("top level variable definitions are not implemented yet");

    default:
        PANIC("invalid token %s (%s)", tok->token, token_type_str(tok->type));
    }

    return out;
}

void ast_parse(struct list2c *tokens, struct list2c *top_statements_head)
{
    struct ast_parser parser = {tokens,
                                .current = TOKEN_LIST_GET(tokens->next)};
    while (parser_get_current(&parser)->type != TOKEN_EOF)
    {

        struct ast_top_statement *statement = ast_parse_top_statement(&parser);
        list2c_push(&statement->list, top_statements_head);
    }
}

struct variable
{
    char *varname;
    int value;
};

/* very simple test function for evaluating a simple expression */
int expr_eval(struct ast_expr *expr, struct array variables)
{
    if (expr->type == AST_EXPRESSION_OPERATOR)
    {
        if (!strcmp(expr->operator,
                    "+")) /* TODO: maybe want to have enum for operator */
        {
            int lhs = expr_eval(expr->lhs, variables);
            int rhs = expr_eval(expr->rhs, variables);
            return lhs + rhs;
        }
        else if (!strcmp(expr->operator, "="))
        {
            if (!expr->lhs->value.is_var)
            {
                PANIC("attempt to assign to not variable");
            }

            for (size_t i = 0;
                 i < ARRAY_LEN(variables, sizeof(struct variable)); i++)
            {
                struct variable *var =
                    array_get(&variables, i, sizeof(struct variable));

                if (!strcmp(expr->lhs->value.varname, var->varname))
                {
                    var->value = expr_eval(expr->rhs, variables);
                    return var->value;
                }
            }
            PANIC("variable not found:%s", expr->lhs->value.varname);
        }
        else if (!strcmp(expr->operator, "<"))
        {
            int lhs = expr_eval(expr->lhs, variables);
            int rhs = expr_eval(expr->rhs, variables);
            return lhs < rhs;
        }
        else if (!strcmp(expr->operator, ">"))
        {
            int lhs = expr_eval(expr->lhs, variables);
            int rhs = expr_eval(expr->rhs, variables);
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
                 i < ARRAY_LEN(variables, sizeof(struct variable)); i++)
            {
                struct variable *var =
                    array_get(&variables, i, sizeof(struct variable));

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

                int val = expr_eval(arg->expr, variables);
                logi("print:%d", val);
            }
            return 0; /* TODO: change this when there is a better type
                         system for the interpreter */
        }
        else
        {
            PANIC("unknown function name:%s", expr->call.function_name);
        }
    }
    UNREACHABLE("unknown enum type:%d", expr->type);
}

void statements_eval(struct list2c *statements_head, struct array *variables)
{
    struct list2c *pos;
    list2c_for_each(pos, statements_head)
    {
        struct ast_statement *statement =
            containerof(pos, struct ast_statement, list);

        int value;
        switch (statement->type)
        {
        case AST_STATEMENT_DECLARE:
            logd("declaration of %s", statement->varname);
            int init_val = 0;
            if (statement->init_value != NULL)
            {
                init_val = expr_eval(statement->init_value, *variables);
            }
            struct variable var = {statement->varname, init_val};
            array_push(variables, &var, sizeof(struct variable));
            break;
        case AST_STATEMENT_EXPR:
            expr_eval(statement->expr, *variables);
            break;
        case AST_STATEMENT_IF:;
            value = expr_eval(statement->block_expr, *variables);
            if (value != 0)
            {
                statements_eval(&statement->block, variables);
            }
            break;
        case AST_STATEMENT_WHILE:;
            value = expr_eval(statement->block_expr, *variables);
            while (value != 0)
            {
                statements_eval(&statement->block, variables);
                value = expr_eval(statement->block_expr, *variables);
            }
            break;
        }
    }
}

void statements_eval_simple(struct list2c *statements_head)
{
    struct array vars = ARRAY_INIT();
    statements_eval(statements_head, &vars);

    for (size_t i = 0; i < ARRAY_LEN(vars, sizeof(struct variable)); i++)
    {
        struct variable *var = array_get(&vars, i, sizeof(struct variable));
        logd("variable of name:%s with value:%d", var->varname, var->value);
    }
}

/* FIXME: using the libc version of this function will result in linker
 * errors, and the tests need it, this version should be removed */
char *strcat(char *restrict dest, const char *restrict src)
{
    memcpy(dest + strlen(dest), src, strlen(src) + 1);

    return dest;
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

    "void main(int x, int y) {"
	"int x = 0;"
	"int y = 1;"
	"int z = 2;"
	"int n = 20;"
	"int temp;"	
    "while (z < (n + 1)) {"
	"temp = x + y;"
    "x = y;"
	"y = temp;"
	"z = z + 1;"
    "}"
	"print(y);"
    "}"

    , &tokens);

    // clang-format on
    /* ast_parse_tokens(&tokens); */

    LIST2C_DEFINE_HEAD(top_statements_head);

    ast_parse(&tokens, &top_statements_head);
    /* statements_eval_simple(&statements_head); */

#ifdef TESTS
    run_tests();
#endif

    exit(0);

    return 0;
}
