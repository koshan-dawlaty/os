#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <lib/array.h>
#include <lib/list.h>
#include <stddef.h>
#include <types.h>

enum token_type
{
    TOKEN_IDENTIFIER,
    TOKEN_RETURN,
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
        AST_STATEMENT_RETURN,
    } type;

    union
    {
        /* FIXME: for return maybe could make this its own type */
        /* type is expr or return */
        /* if it is return it can be null,
         * to indicate return from a void function
         */
        struct ast_expr *expr;

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

/* function argument definition */
struct ast_function_arg_def
{
    struct ast_type *type;
    char *name;
    struct list2c list;
};

struct ast_function_declare
{
    struct ast_type *return_type;
    char *name;
    struct list2c args_head;

    struct list2c block;
};

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

void ast_parse(struct list2c *tokens, struct list2c *top_statements_head);

/* interpreter part, maybe move this later */

struct scope
{
    struct list2c list;
    struct array vars;
};

struct interpreter
{
    /* the current scopes the interpreter is in for one function, when a
     * function is called, the scope of that function will be stored on the
     * stack, to not interfere with the scope of the called function  */
    struct list2c all_scopes;
    struct list2c *current_scope;
    struct array funcs; /* this should probably used a linked list, as it is
                           currently storing pointers */
};

static inline struct interpreter *interpreter_init()
{
    struct interpreter *out = malloc(sizeof(struct interpreter));
    out->funcs = ARRAY_INIT();
    list2c_init_head(&out->all_scopes);
    out->current_scope = &out->all_scopes;
    return out;
}

int statements_eval(struct list2c *statements_head, struct interpreter *);
int expr_eval(struct ast_expr *expr, struct interpreter *);

#endif
