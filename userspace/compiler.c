#include "container_of.h"
#include "lib/array.h"
#include "lib/assert.h"
#include "lib/font.h"
#include "sys/types.h"
#include <init.h>
#include <lib/list.h>
#include <lib/log.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/tests.h>

enum token_type
{
    TOKEN_IDENTIFIER,
    TOKEN_UNKNOWN,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_EQUALS,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_INT,
    TOKEN_SEMICOLON,
    TOKEN_O_PAREN,
    TOKEN_C_PAREN,
    TOKEN_O_BRACE,
    TOKEN_C_BRACE,
    TOKEN_EOF,
    TOKEN_IF,
    TOKEN_WHILE,

};
bool token_is_operator(enum token_type type)
{
    return type == TOKEN_ASSIGN || type == TOKEN_PLUS ||
           type == TOKEN_GREATER || type == TOKEN_LESS;
}

const char *token_type_str(enum token_type type)
{
    switch (type)
    {
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

struct token
{
    enum token_type type;
    char *token;
    size_t len;
    struct list2c list;
};

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

struct tokenizer_test_input
{
    const char *str;
    enum token_type type;
};

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

struct test_result *tokenizer_test_impl(const char *program,
                                        struct tokenizer_test_input expected[],
                                        size_t expected_len)
{
    LIST2C_DEFINE_HEAD(token_list);
    tokenizer_parse_tokens(program, &token_list);

    size_t i = 0;
    struct list2c *pos;
    list2c_for_each(pos, &token_list)
    {
        struct token *tok = containerof(pos, struct token, list);

        if (i >= expected_len)
        {

            FAIL_TEST("%d more tokens than expected %d in output", i,
                      expected_len - 1);
        }

        if (tok->type == TOKEN_UNKNOWN)
        {
            FAIL_TEST("unknown token:%s", tok->token);
        }

        if (expected[i].type != tok->type)
        {
            FAIL_TEST("expected token of type:%s, got %s",
                      token_type_str(expected[i].type),
                      token_type_str(tok->type));
        }

        if (strcmp(expected[i].str, tok->token))
        {
            FAIL_TEST("expected token of value:|%s|, got |%s|", expected[i].str,
                      tok->token);
        }

        i++;
    }

    if (i != expected_len)
    {
        FAIL_TEST("less tokens than expected in output");
    }
    TEST_OK();
}

#define tokenizer_test(name, program, ...)                                     \
    TEST(name, {                                                               \
        return tokenizer_test_impl(                                            \
            program, ((struct tokenizer_test_input[]){__VA_ARGS__}),           \
            sizeof(((struct tokenizer_test_input[]){__VA_ARGS__})) /           \
                sizeof(struct tokenizer_test_input));                          \
    })

// clang-format off
tokenizer_test(tokenizer_test1, "int   x   =   10   ;    ",
			   {"int", TOKEN_IDENTIFIER},
			   {"x", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"10", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},
			   {"", TOKEN_EOF},
			   );


tokenizer_test(tokenizer_test2, "int test123=10; int x = 123;",
			   {"int", TOKEN_IDENTIFIER},
			   {"test123", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"10", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},

			   {"int", TOKEN_IDENTIFIER},
			   {"x", TOKEN_IDENTIFIER},
			   {"=", TOKEN_ASSIGN},
			   {"123", TOKEN_INT},
			   {";", TOKEN_SEMICOLON},
			   {"", TOKEN_EOF},
			   );

tokenizer_test(tokenizer_test3, 
		"void main(int argc) {"
		"	print(123);"
		"}",

			   {"void", TOKEN_IDENTIFIER},
			   {"main", TOKEN_IDENTIFIER},
			   {"(", TOKEN_O_PAREN},
			   {"int", TOKEN_IDENTIFIER},
			   {"argc", TOKEN_IDENTIFIER},
			   {")", TOKEN_C_PAREN},
			   {"{", TOKEN_O_BRACE},
			   {"print", TOKEN_IDENTIFIER},
			   {"(", TOKEN_O_PAREN},
			   {"123", TOKEN_INT},
			   {")", TOKEN_C_PAREN},
			   {";", TOKEN_SEMICOLON},
			   {"}", TOKEN_C_BRACE},
			   {"", TOKEN_EOF},

			   );
// clang-format on

enum ast_node_type
{
    AST_BLOCK,
    AST_STATEMENT,
    AST_DEF, /* definition */
    AST_INVALID,
};

struct ast_node
{
    enum ast_node_type type;
    void *data;
};

struct ast_node *ast_node_alloc(enum ast_node_type type, void *data)
{
    struct ast_node *out = malloc(sizeof(struct ast_node));
    out->type = type;
    out->data = data;
    return out;
}

struct ast_node *ast_node_error(const char *str)
{
    return ast_node_alloc(AST_INVALID, strdup(str));
}

struct ast_type
{
    char *name;
};

struct ast_type *ast_type_alloc(const char *name)
{
    struct ast_type *out = malloc(sizeof(struct ast_type));

    out->name = strdup(name);

    return out;
}

enum ast_parser_state
{
    PARSING_DECLARATION,
    PARSING_STATEMENTS,
};

struct ast_parser
{
    enum ast_parser_state state;
    struct list2c *tokens; /* head all tokens being parsed */
    struct token *current; /* current token */
};

struct ast_value
{
    union
    {
        char *varname;
        u64 literal; /* this is a u64 as it should be the largest type available
                      */
    };
    bool is_var;
};

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

struct ast_expr
{
    union
    {
        struct ast_value value;
        struct
        {
            struct ast_expr *lhs;
            struct ast_expr *rhs;
            char *operator;
        };
    };

    enum ast_expr_type
    {
        AST_EXPRESSION_VALUE,
        AST_EXPRESSION_OPERATOR,
    } type;
};

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

struct ast_statement
{
    enum ast_statement_type
    {
        AST_STATEMENT_EXPR,
        AST_STATEMENT_IF,
        AST_STATEMENT_WHILE,
        AST_STATEMENT_DECLARE,
    } type;

    union
    {
        struct ast_expr *expr; /* type is expr */
        struct
        {
            struct list2c block; /* type is if, while, or for */
            struct ast_expr
                *block_expr; /* used for the expression with the if or while
                                loop, will need to change for for loop */
        };
        struct
        { /* type is declare */

            struct ast_expr *init_value; /* can be null */
            struct ast_type *declared_type;
            char *varname;
        };
    };

    struct list2c list;
};

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

#define TOKEN_LIST_GET(l) containerof(l, struct token, list)

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

struct ast_expr *ast_parse_expression(struct ast_parser *parser);

/* ignores operators, utiltiy */
static struct ast_expr *ast_parse_expression_simple(struct ast_parser *parser)
{

    struct token *tok = parser_advance(parser);

    switch (tok->type)
    {
    case TOKEN_INT:
    case TOKEN_IDENTIFIER:
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
        PANIC("invalid token type:%s for token:%s", token_type_str(tok->type),
              tok->token);
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
        if (tok->type == TOKEN_IDENTIFIER &&
            next->type == TOKEN_IDENTIFIER) /* 2 identifiers in a row means it
         must be a declaration */
        {
            parser_advance(parser);
            parser_advance(parser);
            out = ast_statement_alloc_declaration(ast_type_alloc(tok->token),
                                                  next->token, NULL);
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

struct ast_node *ast_parse_tokens(struct list2c *tokens)
{
    struct ast_parser parser = {.state = PARSING_STATEMENTS,
                                tokens,
                                .current = TOKEN_LIST_GET(tokens->next)};
    LIST2C_DEFINE_HEAD(statements);
    ast_parse_statements(&parser, &statements);

    return NULL;
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
    UNREACHABLE("unknown enum type:%d", expr->type);
}

int expr_eval_simple(struct ast_expr *expr)
{
    return expr_eval(expr, ARRAY_INIT());
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
            struct variable var = {statement->varname, 0};
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

struct test_result *test_ast_expressions(char *expr_str, int expected)
{
    LIST2C_DEFINE_HEAD(tokens);
    tokenizer_parse_tokens(expr_str, &tokens);
    struct ast_parser parser = {.state = PARSING_STATEMENTS,
                                &tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    struct ast_expr *expr = ast_parse_expression(&parser);
    int val = expr_eval_simple(expr);
    ASSERT_EQ(val, expected, "%d != %d for expr %s", val, expected, expr_str);

    TEST_OK();
}

#define TEST_AST_EXPRESSIONS(name, expr, expected)                             \
    TEST(name, { return test_ast_expressions(expr, expected); })

TEST_AST_EXPRESSIONS(expr_test1, "1 + ((2 + 3) + 4) ", 10)
TEST_AST_EXPRESSIONS(expr_test2, "1+1+1+1+1+1", 6)
TEST_AST_EXPRESSIONS(expr_test3, "(((1+2)+2)+2) + 3", 10)
TEST_AST_EXPRESSIONS(expr_test4, "(1+2) + 3", 6)
TEST_AST_EXPRESSIONS(expr_test5, "123", 123)

/* FIXME: using the libc version of this function will result in linker errors,
 * and the tests need it, this version should be removed */
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
    "{"
	"int x;"
	"int y;"
	"int z;"
	"z = 1;"
    "123;"
    "x = 0;"
    "y = 10;"
    "while (x < 11) {"
    "y = x + 3;"
	"x = x + 1;"
    "}"
    "}"
    , &tokens);

    // clang-format on
    ast_parse_tokens(&tokens);

    struct ast_parser parser = {.state = PARSING_STATEMENTS,
                                &tokens,
                                .current = TOKEN_LIST_GET(tokens.next)};
    LIST2C_DEFINE_HEAD(statements_head);
    ast_parse_statements(&parser, &statements_head);
    statements_eval_simple(&statements_head);

#ifdef TESTS
    run_tests();
#endif

    exit(0);

    return 0;
}
