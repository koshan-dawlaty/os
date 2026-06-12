#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <lib/array.h>
#include <lib/list.h>
#include <stddef.h>
#include <types.h>

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
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_O_PAREN,
    TOKEN_C_PAREN,
    TOKEN_O_BRACE,
    TOKEN_C_BRACE,
    TOKEN_EOF,
    TOKEN_IF,
    TOKEN_WHILE,

};
const char *token_type_str(enum token_type type);

struct token
{
    enum token_type type;
    char *token;
    size_t len;
    struct list2c list;
};
#define TOKEN_LIST_GET(l) containerof(l, struct token, list)

struct ast_parser
{

    struct list2c *tokens; /* head all tokens being parsed */
    struct token *current; /* current token */
};

void tokenizer_parse_tokens(const char *program, struct list2c *token_list);

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

struct ast_type
{
    char *name;
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

struct ast_function_arg
{
    struct ast_expr *expr;
    struct list2c list;
};

struct ast_function_call
{
    struct list2c arguments_head;
    char *function_name;
};

struct ast_expr
{
    union
    {
        struct ast_value value;
        struct ast_function_call call;
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
        AST_EXPRESSION_CALL,
        AST_EXPRESSION_OPERATOR,
    } type;
};
struct ast_expr *ast_parse_expression(struct ast_parser *parser);

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

/* interpreter part, maybe move this later */
void statements_eval(struct list2c *statements_head, struct array *variables);
int expr_eval(struct ast_expr *expr, struct array variables);

#endif
