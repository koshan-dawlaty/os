#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/compiler/c_compiler.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/tests.h>

bool token_is_branch_statement(enum token_type type)
{
    return type == TOKEN_IF || type == TOKEN_WHILE || type == TOKEN_FOR;
}

bool token_is_operator(enum token_type type)
{
    return type == TOKEN_ASSIGN || type == TOKEN_PLUS || type == TOKEN_MINUS ||
           type == TOKEN_GREATER || type == TOKEN_LESS ||
           type == TOKEN_EQUALS || type == TOKEN_STAR ||
           type == TOKEN_LESS_EQUAL || type == TOKEN_GREATER_EQUAL ||
           type == TOKEN_NOT_EQUAL;
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
    case TOKEN_FOR:
        return "FOR";
    case TOKEN_CHAR:
        return "CHAR LITERAL";
    case TOKEN_INVALID:
        return "INVALID TOKEN";
    case TOKEN_NOT_EQUAL:
        return "NOT EQUAL";
    case TOKEN_GREATER_EQUAL:
        return "GREATER EQUAL";
    case TOKEN_LESS_EQUAL:
        return "LESS EQUAL";
    case TOKEN_INCREMENT:
        return "INCREMENT (++)";
    case TOKEN_REFERENCE:
        return "REFERENCE";
    }
}

struct token *token_alloc_init(enum token_type type, char *str, size_t len)
{
    struct token *out = malloc(sizeof(struct token));

    out->token = str;
    out->len = len;
    out->type = type;
    out->list = LIST_INIT();

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
    {"++", TOKEN_INCREMENT},
    {"+", TOKEN_PLUS},
    {"-", TOKEN_MINUS},
    {"!=", TOKEN_NOT_EQUAL},
    {">=", TOKEN_GREATER_EQUAL},
    {">", TOKEN_GREATER},
    {"<=", TOKEN_LESS_EQUAL},
    {"<", TOKEN_LESS},
    {"*", TOKEN_STAR},
    {"if", TOKEN_IF},
    {"while", TOKEN_WHILE},
    {",", TOKEN_COMMA},
    {"&", TOKEN_REFERENCE},
    {"return", TOKEN_RETURN},
    {"for", TOKEN_FOR},

    // clang-format on

};

bool token_is_unary_operator(enum token_type type)
{
    return type == TOKEN_STAR || type == TOKEN_INCREMENT ||
           type == TOKEN_REFERENCE;
}

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

    if (*s == 0)
    {
        return token_alloc(TOKEN_EOF, "", 0);
    }

    if (*s == '"')
    {
        size_t i = 1;
        while (s[i] != '"' && s[i] != 0)
        {
            i++;
        }
        *len += i + 1;
        return token_alloc(TOKEN_STRING, s + 1, i - 1);
    }

    if (*s == '\'')
    {
        if (s[1] == 0)
        {
            *len += 1;
            return token_alloc(TOKEN_UNKNOWN, "expected character, got EOF", 0);
        }
        if (s[2] != '\'')
        {
            *len += 2;
            return token_alloc(TOKEN_UNKNOWN, "expected ' in character literal",
                               0);
        }
        *len += 3;
        return token_alloc(TOKEN_CHAR, &s[1], 1);
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
        if (s[1] == '=')
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

void tokenizer_parse_tokens(const char *program, struct list *token_list)
{

    size_t offset = 0;

    size_t len = 0;
    struct token *t = get_token(program, &len);
    offset += len;
    while (t->type != TOKEN_UNKNOWN)
    {

        list_push_end(&t->list, token_list);
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

void ast_type_free(struct ast_type *type)
{
    free(type->name);
    free(type);
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
    else if (tok->type == TOKEN_CHAR)
    {
        return AST_VALUE_LITERAL(AST_LITERAL_INT(*tok->token));
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
    arg->list = LIST_INIT();

    return arg;
}

struct ast_expr *ast_expr_alloc_call(const char *function_name)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));
    expr->type = AST_EXPRESSION_CALL;
    expr->call =
        (struct ast_function_call){.function_name = strdup(function_name)};
    list_init_head(&expr->call.arguments_head);

    return expr;
}

struct ast_expr *ast_expr_alloc_unary_operator(struct ast_expr *e,
                                               const char *operator)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));

    expr->type = AST_EXPRESSION_UNARY_OPERATOR;
    expr->unary_operator.operator = strdup(operator);
    expr->unary_operator.expr = e;

    return expr;
}

struct ast_expr *ast_expr_alloc_operator(struct ast_expr *lhs,
                                         struct ast_expr *rhs,
                                         enum token_type operator)
{
    struct ast_expr *expr = malloc(sizeof(struct ast_expr));

    expr->type = AST_EXPRESSION_OPERATOR;
    expr->operator = operator;
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
    out->list = LIST_INIT();

    return out;
}

struct ast_statement *ast_statement_alloc_return(struct ast_expr *expr)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    out->expr = expr;
    out->type = AST_STATEMENT_RETURN;
    out->list = LIST_INIT();

    return out;
}

void ast_expr_free(struct ast_expr *expr)
{
    switch (expr->type)
    {
    case AST_EXPRESSION_OPERATOR:
        ast_expr_free(expr->lhs);
        ast_expr_free(expr->rhs);
        break;

    case AST_EXPRESSION_UNARY_OPERATOR:
        ast_expr_free(expr->unary_operator.expr);
        free(expr->unary_operator.operator);
        break;

    case AST_EXPRESSION_CALL:
        free(expr->call.function_name);
        break;

    case AST_EXPRESSION_VALUE:
        if (!expr->value.is_var &&
            expr->value.literal.type == AST_LITERAL_TYPE_STRING)
        {
            free(expr->value.literal.str);
        }
        break;
    }
    free(expr);
}

void ast_statements_free(struct list *statements_head);
void ast_statement_free(struct ast_statement *statement)
{
    switch (statement->type)
    {
    case AST_STATEMENT_RETURN:
    case AST_STATEMENT_EXPR:
        ast_expr_free(statement->expr);
        break;
    case AST_STATEMENT_IF:
    case AST_STATEMENT_WHILE:
        ast_expr_free(statement->block_expr);
        ast_statements_free(&statement->block);
        break;
    case AST_STATEMENT_FOR:
        ast_expr_free(statement->for_expr.expr1);
        ast_expr_free(statement->for_expr.expr2);
        ast_expr_free(statement->for_expr.expr3);
        ast_statements_free(&statement->block);
        break;
    case AST_STATEMENT_DECLARE:
        free(statement->varname);
        break;
    }
    free(statement);
}

void ast_statements_free(struct list *statements_head)
{
    struct list *prev = NULL;
    struct list *pos;
    list_for_each(pos, statements_head)
    {
        if (prev != NULL)
        {
            ast_statement_free(containerof(prev, struct ast_statement, list));
        }
        prev = pos;
    }
    if (prev != NULL)
    {
        ast_statement_free(containerof(prev, struct ast_statement, list));
    }
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
    out->list = LIST_INIT();

    return out;
}

struct ast_statement *ast_statement_alloc_block_for(bool first_is_statement)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    list_init_head(&out->block);
    out->type = AST_STATEMENT_FOR;
    out->for_expr.first_is_statement = first_is_statement;
    out->list = LIST_INIT();

    return out;
}

struct ast_statement *ast_statement_alloc_block(enum ast_statement_type type,
                                                struct ast_expr *block_expr)
{
    struct ast_statement *out = malloc(sizeof(struct ast_statement));
    list_init_head(&out->block);
    out->type = type;
    out->block_expr = block_expr;
    out->list = LIST_INIT();

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

    struct list *pos = parser_get_current(parser)->list.next;
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

    if (token_is_unary_operator(tok->type))
    {
        /* TODO: also have postfix operators */

        logd("parsing expression because there was a unary operator");
        /*
         * this uses the simple version of this function because, in
         * some cases, there can be operators after the unary operator
         * application that should be excluded from the unary operator
         * node in the ast. calling ast_parse_expression_simple() here
         * instead will not parse the binary operators after the left
         * hand side of the expression  that ast_parse_expression() does
         */
        struct ast_expr *expr = ast_parse_expression_simple(parser, err);
        if (expr == NULL)
        {
            return NULL;
        }

        return ast_expr_alloc_unary_operator(expr, tok->token);
    }

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
                list_push_end(&arg->list, &call->arguments_head);

                struct token *tok = parser_advance(parser);

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
    case TOKEN_CHAR:
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

/* FIXME: duplicated in struct ast_expr */
struct ast_operator
{
    /* expression with type of AST_EXPRESSION_OPERATOR */
    struct ast_expr *expr;
    struct list list;
};

static const enum token_type operator_table[][10] = {
    // clang-format off
    {TOKEN_STAR, TOKEN_INVALID},
	{TOKEN_PLUS, TOKEN_MINUS, TOKEN_INVALID},
	{TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_INVALID},
	{TOKEN_EQUALS, TOKEN_NOT_EQUAL, TOKEN_INVALID},
    /* {"&&", TOKEN_INVALID}, */
	/* {"||", TOKEN_INVALID}, */
	{TOKEN_ASSIGN, TOKEN_INVALID},
    // clang-format on
};

static void print_op(struct ast_operator *operator)
{
    logd("lhs:%d, operator:%d rhs:%d",
         operator->expr->lhs->value.literal.integer, operator->expr->operator,
         operator->expr->rhs->value.literal.integer);
}

struct ast_operator *ast_operator_alloc(struct ast_expr *expr)
{
    struct ast_operator *out = malloc(sizeof(struct ast_operator));
    out->expr = expr;
    out->list = LIST_INIT();
    return out;
}

struct ast_expr *ast_parse_expression(struct ast_parser *parser,
                                      struct ast_error **err)
{
    struct ast_expr *retval = NULL;
    bool first_operator = true;
    LIST_DEFINE_HEAD(operators);
    struct ast_expr *lhs = ast_parse_expression_simple(parser, err);
    while (true)
    {
        struct token *operator_token = parser_get_current(parser);
        if (!token_is_operator(operator_token->type))
        {
            if (first_operator)
            {
                retval = lhs;
                goto end;
            }
            else
            {
                break;
            }
        }
        parser_advance(parser);
        struct ast_expr *rhs = ast_parse_expression_simple(parser, err);
        struct ast_operator *operator = ast_operator_alloc(
            ast_expr_alloc_operator(lhs, rhs, operator_token->type));
        list_push_end(&operator->list, &operators);
        lhs = rhs;
        first_operator = false;
    }
    struct list *pos;
    list_for_each(pos, &operators)
    {
        struct ast_operator *op = containerof(pos, struct ast_operator, list);
    }

    for (size_t i = 0; i < sizeof(operator_table) / sizeof(operator_table[i]);
         i++)
    {

        const enum token_type *table_row = operator_table[i];
        struct list *pos;
        for (pos = (&operators)->next; pos != (&operators);)
        {
            struct ast_operator *next = NULL, *prev = NULL;
            if (pos->next != &operators)
            {
                next = containerof(pos->next, struct ast_operator, list);
            }
            if (pos->prev != &operators)
            {
                prev = containerof(pos->prev, struct ast_operator, list);
            }

            struct ast_operator *op =
                containerof(pos, struct ast_operator, list);
            size_t j = 0;
            while (table_row[j] != TOKEN_INVALID)
            {

                if (op->expr->operator == table_row[j])
                {
                    if (prev != NULL)
                    {
                        struct ast_expr *temp = prev->expr->rhs;
                        prev->expr->rhs = op->expr;
                        op->expr->lhs = temp;
                    }
                    if (next != NULL)
                    {
                        struct ast_expr *temp = next->expr->lhs;
                        next->expr->lhs = op->expr;
                        op->expr->rhs = temp;
                    }
                    struct list *temp = pos;
                    pos = pos->next;
                    if (operators.next->next != &operators)
                    {
                        list_remove(temp);
                    }
                    goto loop_end;
                }
                j++;
            }
            pos = pos->next;
        loop_end:;
        }
    }

    struct ast_operator *out =
        containerof(operators.next, struct ast_operator, list);
    retval = out->expr;
end:;
    struct token *end_token = parser_get_current(parser);
    if (token_is_unary_operator(end_token->type))
    {
        retval = ast_expr_alloc_unary_operator(retval, end_token->token);
        parser_advance(parser);
    }

    /* logd("returning:%p", out->expr); */
    return retval;
}

int ast_parse_statements(struct ast_parser *parser,
                         struct list *statements_head, struct ast_error **err);
struct ast_statement *ast_parse_statement_branch(struct ast_parser *parser,
                                                 struct ast_error **err)
{
    struct ast_statement *out = NULL;
    struct token *tok = parser_advance(parser);

    struct token *open_paren = parser_advance(parser);
    if (open_paren->type != TOKEN_O_PAREN)
    {
        *err = ast_error_alloc(AST_ERROR_OPEN_PAREN_EXPECTED);
        (*err)->open_paren_expected.token = open_paren;
        return NULL;
    }

    if (tok->type == TOKEN_IF)
    {
        out = ast_statement_alloc_block(AST_STATEMENT_IF,
                                        ast_parse_expression(parser, err));
    }
    else if (tok->type == TOKEN_WHILE)
    {
        out = ast_statement_alloc_block(AST_STATEMENT_WHILE,
                                        ast_parse_expression(parser, err));
    }
    else if (tok->type == TOKEN_FOR)
    {

        struct token *type_end;
        if (parser_next_is_type(parser, &type_end) &&
            type_end->type == TOKEN_IDENTIFIER)
        {
            struct ast_type *type = parser_parse_type(parser, err);
            /* variable name */
            parser_advance(parser);
            struct token *assign = parser_advance(parser);
            if (assign->type != TOKEN_ASSIGN)
            {
                /* it might techincally be wrong to panic here */
                PANIC("for loop init value not initialized");
            }

            struct ast_statement *first = ast_statement_alloc_declaration(
                type, type_end->token, ast_parse_expression(parser, err));
            out = ast_statement_alloc_block_for(true);
            out->for_expr.statement1 = first;
        }
        else
        {
            logd("not a declaration");
            out = ast_statement_alloc_block_for(false);
            out->for_expr.expr1 = ast_parse_expression(parser, err);
            if (out->for_expr.expr1 == NULL)
            {
                return NULL;
            }
        }

        struct token *semicolon = parser_advance(parser);

        if (semicolon->type != TOKEN_SEMICOLON)
        {
            *err = ast_error_alloc(AST_ERROR_SEMICOLON_EXPECTED);
            (*err)->semicolon_expected.token = semicolon;
            return NULL;
        }
        out->for_expr.expr2 = ast_parse_expression(parser, err);
        if (out->for_expr.expr2 == NULL)
        {
            return NULL;
        }
        semicolon = parser_advance(parser);
        if (semicolon->type != TOKEN_SEMICOLON)
        {
            *err = ast_error_alloc(AST_ERROR_SEMICOLON_EXPECTED);
            (*err)->semicolon_expected.token = semicolon;
            return NULL;
        }
        out->for_expr.expr3 = ast_parse_expression(parser, err);

        if (out->for_expr.expr3 == NULL)
        {
            return NULL;
        }
    }

    struct token *close_paren = parser_advance(parser);
    if (close_paren->type != TOKEN_C_PAREN)
    {
        *err = ast_error_alloc(AST_ERROR_CLOSE_PAREN_EXPECTED);
        (*err)->open_paren_expected.token = open_paren;
        return NULL;
    }

    if (ast_parse_statements(parser, &out->block, err))
    {
        return NULL;
    }
    return out;
}

struct ast_statement *ast_parse_statement(struct ast_parser *parser,
                                          struct ast_error **err)
{
    struct token *tok = parser_get_current(parser);

    struct ast_statement *out = NULL;

    if (tok->type == TOKEN_IDENTIFIER || tok->type == TOKEN_INT ||
        token_is_unary_operator(tok->type)) /* TODO: this might not work
with other kinds of statements */
    {
        struct token *type_end = NULL;
        struct token *next = parser_get_next(parser);
        bool next_is_type = parser_next_is_type(parser, &type_end);
        /* TODO: wont work with unsigned types, as it checks that
         * next->type is a variable */
        if (next_is_type && type_end->type == TOKEN_IDENTIFIER) /* 2
            identifiers in a row means it must be a declaration */
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
        else if (next->type != TOKEN_IDENTIFIER ||
                 token_is_unary_operator(tok->type))
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
    else if (token_is_branch_statement(tok->type))
    {
        return ast_parse_statement_branch(parser, err);
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
    else
    {
        PANIC("unknown statement type for token %s (%s)", tok->token,
              token_type_str(tok->type));
    }

    return out;
}

int ast_parse_statements(struct ast_parser *parser,
                         struct list *statements_head, struct ast_error **err)
{

    struct token *open_brace = parser_advance(parser);
    if (open_brace->type != TOKEN_O_BRACE)
    {
        *err = ast_error_alloc(AST_ERROR_OPEN_BRACE_EXPECTED);
        return -1;
    }

    while (parser_get_current(parser)->type != TOKEN_C_BRACE)
    {
        struct ast_statement *statement = ast_parse_statement(parser, err);
        if (statement == NULL)
        {
            return -1;
        }
        list_push_end(&statement->list, statements_head);
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
    out->list = LIST_INIT();

    return out;
}

void ast_function_arg_def_free(struct ast_function_arg_def *arg)
{
    ast_type_free(arg->type);
    free(arg->name);
    free(arg);
}

struct ast_function_declare *
ast_function_declare_alloc(struct ast_type *return_type, const char *name)
{
    struct ast_function_declare *out =
        malloc(sizeof(struct ast_function_declare));
    out->name = strdup(name);
    list_init_head(&out->args_head);
    list_init_head(&out->block);
    out->return_type = return_type;

    return out;
}

void ast_function_declare_free(struct ast_function_declare *declare)
{
    ast_statements_free(&declare->block);
    free(declare->name);
    ast_type_free(declare->return_type);
    struct list *prev = NULL;
    struct list *pos;
    list_for_each(pos, &declare->args_head)
    {
        if (prev != NULL)
        {
            ast_function_arg_def_free(
                containerof(prev, struct ast_function_arg_def, list));
        }
        prev = pos;
    }
    if (prev != NULL)
    {
        ast_function_arg_def_free(
            containerof(prev, struct ast_function_arg_def, list));
    }

    free(declare);
}

struct ast_top_statement *ast_parse_top_statement(struct ast_parser *parser,
                                                  struct ast_error **err)
{
    struct ast_top_statement *out = malloc(sizeof(struct ast_top_statement));
    out->list = LIST_INIT();
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
            list_push_end(&arg->list, &func->args_head);
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

        struct list *pos;
        list_for_each(pos, &func->block)
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

void ast_top_statement_free(struct ast_top_statement *statement)
{
    switch (statement->type)
    {
    case AST_TOP_STATEMENT_FUNC:
        ast_function_declare_free(statement->function);
        break;
    case AST_TOP_STATEMENT_VAR:
        PANIC("ast top level variables are not implemented");
    case AST_TOP_STATEMENT_TYPE:
        PANIC("ast top level types are not implemented");
    }
    free(statement);
}

void ast_top_statements_free(struct list *top_statements_head)
{
    struct list *prev = NULL;
    struct list *pos;
    list_for_each(pos, top_statements_head)
    {
        if (prev != NULL)
        {
            ast_top_statement_free(
                containerof(prev, struct ast_top_statement, list));
        }
        prev = pos;
    }
    if (prev != NULL)
    {
        ast_top_statement_free(
            containerof(prev, struct ast_top_statement, list));
    }
}

struct ast_error *ast_parse(struct list *tokens,
                            struct list *top_statements_head)
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
            return error;
        }
        list_push(&statement->list, top_statements_head);
    }
    return NULL;
}

int ast_error_to_string(struct ast_error *err, char *buf, size_t len)
{

    switch (err->type)
    {
    case AST_ERROR_IDENTIFIER_EXPECTED:
        return snprintf(buf, len, "expected identifier, got %s (%s)",
                        err->identifier_expected.token->token,
                        token_type_str(err->identifier_expected.token->type));
    case AST_ERROR_ARUGMENT_EXPECTED:
        return snprintf(buf, len, "expected ',' or ')' got %s (%s)",
                        err->argument_expected.token->token,
                        token_type_str(err->argument_expected.token->type));
    case AST_ERROR_INVALID_TYPE:
        return snprintf(buf, len, "invalid type for token %s (%s)",
                        err->invalid_type.token->token,
                        token_type_str(err->invalid_type.token->type));
    case AST_ERROR_OPEN_BRACE_EXPECTED:
        return snprintf(buf, len, "expected '}' at the start of a block");
    case AST_ERROR_SEMICOLON_EXPECTED:
        return snprintf(buf, len, "expected ';' got %s (%s)",
                        err->semicolon_expected.token->token,
                        token_type_str(err->semicolon_expected.token->type));
    case AST_ERROR_OPEN_PAREN_EXPECTED:
        return snprintf(buf, len, "expected '(' got %s (%s)",
                        err->close_paren_expected.token->token,
                        token_type_str(err->close_paren_expected.token->type));
    case AST_ERROR_CLOSE_PAREN_EXPECTED:
        return snprintf(buf, len, "expected ')' got %s (%s)",
                        err->close_paren_expected.token->token,
                        token_type_str(err->close_paren_expected.token->type));
    case AST_ERROR_INVALID_EXPRESSION_TOKEN:
        return snprintf(
            buf, len, "invalid token %s (%s) when parsing expression",
            err->invalid_expression_token.token->token,
            token_type_str(err->invalid_expression_token.token->type));
    case AST_ERROR_DECLARATION_OR_EXPRESSION_EXPECTED:
        return snprintf(buf, len, "expected declaration or expression");
    default:
        return snprintf(buf, len, "unknown ast error %d", err->type);
    }
}
