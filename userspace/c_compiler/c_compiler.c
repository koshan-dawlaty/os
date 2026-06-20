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
    return type == TOKEN_ASSIGN || type == TOKEN_PLUS || type == TOKEN_MINUS ||
           type == TOKEN_GREATER || type == TOKEN_LESS;
}

const char *token_type_str(enum token_type type)
{
    switch (type)
    {
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_RETURN:
        return "RETURN";
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
    case TOKEN_MINUS:
        return "MINUS";
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
    {"-", TOKEN_MINUS},
    {">", TOKEN_GREATER},
    {"<", TOKEN_LESS},
    {"*", TOKEN_STAR},
    {"if", TOKEN_IF},
    {"while", TOKEN_WHILE},
    {",", TOKEN_COMMA},
    {"return", TOKEN_RETURN},

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

    if (*s == '"')
    {
        size_t i = 1;
        while (s[i] != '"' && s[i] != 0)
        {
            logd("%c", s[i]);
            i++;
        }
        *len += i + 1;
        return token_alloc(TOKEN_STRING, s + 1, i - 1);
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

struct ast_type *ast_type_alloc(const char *name, size_t num_pointers)
{
    struct ast_type *out = malloc(sizeof(struct ast_type));

    out->name = strdup(name);
    out->num_pointers = num_pointers;

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
        return AST_VALUE_LITERAL(AST_LITERAL_INT(num));
    }
    else if (tok->type == TOKEN_STRING)
    {
        return AST_VALUE_LITERAL(AST_LITERAL_STRING(tok->token));
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

struct ast_statement *ast_statement_alloc_return(struct ast_expr *expr)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    out->expr = expr;
    out->type = AST_STATEMENT_RETURN;
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

bool parser_next_is_type(struct ast_parser *parser, struct token **token)
{
    struct token *type = parser_get_current(parser);

    struct list2c *pos = parser_get_current(parser)->list.next;
    struct token *tok = containerof(pos, struct token, list);

    while (tok->type == TOKEN_STAR)

    {
        pos = pos->next;
        tok = containerof(pos, struct token, list);
    }
    /* pos = pos->next; */
    /* tok = containerof(pos, struct token, list); */

    *token = tok;

    return type->type == TOKEN_IDENTIFIER;
}

struct ast_type *parser_parse_type(struct ast_parser *parser,
                                   struct ast_error **err)
{
    struct token *next_end;

    if (!parser_next_is_type(parser, &next_end))
    {
        *err = ast_error_alloc(AST_ERROR_INVALID_TYPE);
        (*err)->invalid_type.token = parser_get_current(parser);
        return NULL;
    }

    struct token *typename = parser_advance(parser);

    size_t num_pointers = 0;
    while (parser_get_current(parser)->type == TOKEN_STAR)
    {

        num_pointers++;
        parser_advance(parser);
    }

    return ast_type_alloc(typename->token, num_pointers);
}

/* ignores operators, utiltiy */
static struct ast_expr *ast_parse_expression_simple(struct ast_parser *parser,
                                                    struct ast_error **err)
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
                struct ast_expr *expr = ast_parse_expression(parser, err);
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
                    *err = ast_error_alloc(AST_ERROR_ARUGMENT_EXPECTED);
                    (*err)->argument_expected.token = tok;
                    return NULL;
                }
            }
            /* parser_advance(parser); */
            return out;
        }
    case TOKEN_INT:
    case TOKEN_STRING:
        return ast_expr_alloc_value(ast_token_to_value(tok));

    case TOKEN_O_PAREN:;
        struct ast_expr *out = ast_parse_expression(parser, err);
        struct token *close_paren = parser_advance(parser);
        if (close_paren->type != TOKEN_C_PAREN)
        {
            *err = ast_error_alloc(AST_ERROR_CLOSE_PAREN_EXPECTED);
            (*err)->close_paren_expected.token = close_paren;
            return NULL;
        }
        return out;

    default:
        /* TODO: if it is a keyword, error */
        *err = ast_error_alloc(AST_ERROR_INVALID_EXPRESSION_TOKEN);
        (*err)->invalid_expression_token.token = tok;
        return NULL;
    }
}

struct ast_expr *ast_parse_expression(struct ast_parser *parser,
                                      struct ast_error **err)
{
    /* TODO:!!! order of operations */

    struct ast_expr *lhs = ast_parse_expression_simple(
        parser, err); /* the left hand side should always be an expression that
                    does not require recursion */

    if (lhs == NULL)
    {
        return NULL;
    }

    struct token *operator_token = parser_get_current(parser);
    if (token_is_operator(operator_token->type))
    {
        parser_advance(parser);
        struct ast_expr *rhs = ast_parse_expression(parser, err);
        if (rhs == NULL)
        {
            return NULL;
        }

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

int ast_parse_statements(struct ast_parser *parser,
                         struct list2c *statements_head,
                         struct ast_error **err);

struct ast_statement *ast_parse_statement(struct ast_parser *parser,
                                          struct ast_error **err)
{
    struct token *tok = parser_get_current(parser);

    struct ast_statement *out = NULL;
    /* logd("%s, %s", tok->token, token_type_str(tok->type)); */

    if (tok->type == TOKEN_IDENTIFIER ||
        tok->type == TOKEN_INT) /* TODO: this might not work with
         other kinds of statements */
    {
        struct token *type_end = NULL;
        struct token *next = parser_get_next(parser);
        bool next_is_type = parser_next_is_type(parser, &type_end);
        /* TODO: wont work with unsigned types, as it checks that next->type
         * is a variable */
        if (next_is_type && type_end->type == TOKEN_IDENTIFIER) /* 2 identifiers
            in a row means it must be a declaration */
        {
            struct ast_type *type =
                parser_parse_type(parser, NULL); /* TODO: dont pass null */
            const char *name = type_end->token;
            struct ast_expr *init_val = NULL;
            parser_advance(parser);
            if (parser_get_current(parser)->type == TOKEN_ASSIGN)
            {
                parser_advance(parser);
                init_val = ast_parse_expression(parser, err);
            }
            out = ast_statement_alloc_declaration(type, name, init_val);
        }
        else if (next->type != TOKEN_IDENTIFIER)
        {
            out = ast_statement_alloc_expr(ast_parse_expression(parser, err));
        }
        else
        {
            *err =
                ast_error_alloc(AST_ERROR_DECLARATION_OR_EXPRESSION_EXPECTED);
            return NULL;
        }

        struct token *semicolon = parser_advance(parser);
        if (semicolon->type != TOKEN_SEMICOLON)
        {
            *err = ast_error_alloc(AST_ERROR_SEMICOLON_EXPECTED);
            (*err)->semicolon_expected.token = semicolon;
            return NULL;
        }
    }
    else if (tok->type == TOKEN_IF)
    {
        parser_advance(parser);
        out = ast_statement_alloc_block(
            AST_STATEMENT_IF,
            ast_parse_expression(
                parser,
                err) /* TODO: this will allow things like if x == y {} */);

        if (ast_parse_statements(parser, &out->block, err))
        {
            return NULL;
        }
    }
    else if (tok->type == TOKEN_WHILE)
    {
        parser_advance(parser);
        out = ast_statement_alloc_block(
            AST_STATEMENT_WHILE,
            ast_parse_expression(
                parser,
                err) /* TODO: this will allow things like if x == y {} */);

        if (ast_parse_statements(parser, &out->block, err))
        {
            return NULL;
        }
    }
    else if (tok->type == TOKEN_RETURN)
    {
        parser_advance(parser);

        if (parser_get_current(parser)->type == TOKEN_SEMICOLON)
        {
            out = ast_statement_alloc_return(NULL);
        }
        else
        {
            struct ast_expr *expr = ast_parse_expression(parser, err);
            out = ast_statement_alloc_return(expr);

            struct token *semicolon = parser_advance(parser);
            if (semicolon->type != TOKEN_SEMICOLON)
            {
                *err = ast_error_alloc(AST_ERROR_SEMICOLON_EXPECTED);
                (*err)->semicolon_expected.token = semicolon;
                return NULL;
            }
        }
    }

    return out;
}

int ast_parse_statements(struct ast_parser *parser,
                         struct list2c *statements_head, struct ast_error **err)
{

    struct token *open_brace = parser_advance(parser);
    if (open_brace->type != TOKEN_O_BRACE)
    {
        *err = ast_error_alloc(AST_ERROR_OPEN_BRACE_EXPECTED);
    }

    while (parser_get_current(parser)->type != TOKEN_C_BRACE)
    {
        struct ast_statement *statement = ast_parse_statement(parser, err);
        if (statement == NULL)
        {
            return -1;
        }
        list2c_push_end(&statement->list, statements_head);
    }
    parser_advance(parser);

    return 0;
}

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

struct ast_top_statement *ast_parse_top_statement(struct ast_parser *parser,
                                                  struct ast_error **err)
{
    struct ast_top_statement *out = malloc(sizeof(struct ast_top_statement));
    out->list = LIST2C_INIT();
    struct ast_type *type = parser_parse_type(
        parser, err); /* will not work for parsing type definitions */
    if (type == NULL)
    {
        return NULL;
    }

    struct token *name = parser_advance(parser);
    if (name->type != TOKEN_IDENTIFIER)
    {
        *err = ast_error_alloc(AST_ERROR_IDENTIFIER_EXPECTED);
        (*err)->identifier_expected.token = name;
        return NULL;
    }

    struct token *tok = parser_get_current(parser);

    switch (tok->type)
    {

    case TOKEN_O_PAREN:;
        parser_advance(parser);
        struct ast_function_declare *func =
            ast_function_declare_alloc(type, name->token);

        if (parser_get_current(parser)->type == TOKEN_C_PAREN)
        {
            goto args_done;
        }

        while (true)
        {

            struct ast_type *type = parser_parse_type(parser, err);
            if (type == NULL)
            {
                return NULL;
            }
            struct token *name = parser_advance(parser);
            if (name->type != TOKEN_IDENTIFIER)
            {

                *err = ast_error_alloc(AST_ERROR_IDENTIFIER_EXPECTED);
                (*err)->identifier_expected.token = name;
                return NULL;
            }

            struct ast_function_arg_def *arg =
                ast_function_arg_def_alloc(type, name->token);
            list2c_push_end(&arg->list, &func->args_head);
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
                *err = ast_error_alloc(AST_ERROR_ARUGMENT_EXPECTED);
                (*err)->argument_expected.token = current;
                return NULL;
            }
        }
    args_done:
        parser_advance(parser); /* ')' */

        if (ast_parse_statements(parser, &func->block, err))
        {
            return NULL;
        }

        struct list2c *pos;
        list2c_for_each(pos, &func->block)
        {
            struct ast_statement *statement =
                containerof(pos, struct ast_statement, list);
        }

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

struct ast_error *ast_parse(struct list2c *tokens,
                            struct list2c *top_statements_head)
{
    struct ast_parser parser = {tokens,
                                .current = TOKEN_LIST_GET(tokens->next)};
    while (parser_get_current(&parser)->type != TOKEN_EOF)
    {

        struct ast_error *error;
        struct ast_top_statement *statement =
            ast_parse_top_statement(&parser, &error);

        if (statement == NULL)
        {
            ast_error_to_string(error);
            return error;
        }
        list2c_push(&statement->list, top_statements_head);
    }
    return NULL;
}

char *ast_error_to_string(struct ast_error *err)
{

    switch (err->type)
    {
    case AST_ERROR_IDENTIFIER_EXPECTED:
        loge("expected identifier, got %s (%s)",
             err->identifier_expected.token->token,
             token_type_str(err->identifier_expected.token->type));
        break;
    case AST_ERROR_ARUGMENT_EXPECTED:
        loge("expected ',' or ')' got %s (%s)",
             err->argument_expected.token->token,
             token_type_str(err->argument_expected.token->type));
        break;
    case AST_ERROR_INVALID_TYPE:
        loge("invalid type for token %s (%s)", err->invalid_type.token->token,
             token_type_str(err->invalid_type.token->type));
        break;
    case AST_ERROR_OPEN_BRACE_EXPECTED:
        loge("expected '}' at the start of a block");
        break;
    case AST_ERROR_SEMICOLON_EXPECTED:
        loge("expected ';' got %s (%s)", err->semicolon_expected.token->token,
             token_type_str(err->semicolon_expected.token->type));
        break;
    case AST_ERROR_CLOSE_PAREN_EXPECTED:
        loge("expected ')' got %s (%s)", err->close_paren_expected.token->token,
             token_type_str(err->close_paren_expected.token->type));
        break;
    case AST_ERROR_INVALID_EXPRESSION_TOKEN:
        loge("invalid token %s (%s) when parsing expression",
             err->invalid_expression_token.token->token,
             token_type_str(err->invalid_expression_token.token->type));
        break;
    case AST_ERROR_DECLARATION_OR_EXPRESSION_EXPECTED:
        loge("expected declaration or expression");
        break;
    }

    return "";
}

/* FIXME: using the libc version of this function will result in linker
 * errors, and the tests need it, this version should be removed */
char *strcat(char *restrict dest, const char *restrict src)
{
    memcpy(dest + strlen(dest), src, strlen(src) + 1);

    return dest;
}
