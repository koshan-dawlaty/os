#ifndef C_COMPILER_H
#define C_COMPILER_H

#include "lib/log.h"
#include <lib/array.h>
#include <lib/list.h>
#include <stddef.h>
#include <types.h>

enum token_type
{
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_RETURN,
    TOKEN_UNKNOWN,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
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
    TOKEN_STAR,

};
const char *token_type_str(enum token_type type);

struct token
{
    enum token_type type;
    char *token;
    size_t len;
    struct list list;
};
#define TOKEN_LIST_GET(l) containerof(l, struct token, list)

struct ast_parser
{

    struct list *tokens;   /* head all tokens being parsed */
    struct token *current; /* current token */
};

enum ast_error_type
{
    AST_ERROR_IDENTIFIER_EXPECTED,
    AST_ERROR_ARUGMENT_EXPECTED,
    AST_ERROR_INVALID_TYPE,
    AST_ERROR_OPEN_BRACE_EXPECTED,
    AST_ERROR_CLOSE_PAREN_EXPECTED,
    AST_ERROR_SEMICOLON_EXPECTED,
    AST_ERROR_DECLARATION_OR_EXPRESSION_EXPECTED,
    AST_ERROR_INVALID_EXPRESSION_TOKEN,
};
struct ast_error
{

    enum ast_error_type type;
    union
    {
        struct
        {
            struct token *token;
        } identifier_expected;
        struct
        {
            struct token *token;
        } argument_expected;
        struct
        {
            struct token *token;
        } invalid_type;
        struct
        {
            struct token *token;
        } semicolon_expected;
        struct
        {
            struct token *token;
        } close_paren_expected;
        struct
        {
            struct token *token;
        } invalid_expression_token;
    };
};

int ast_error_to_string(struct ast_error *err, char *buf, size_t len);

static inline struct ast_error *ast_error_alloc(enum ast_error_type type)
{
    struct ast_error *err = malloc(sizeof(struct ast_error));
    err->type = type;

    return err;
}

void tokenizer_parse_tokens(const char *program, struct list *token_list);

struct ast_type
{
    size_t num_pointers;
    char *name;
};
struct ast_type *ast_type_alloc(const char *name, size_t num_pointers);

struct ast_literal
{
    enum ast_literal_type
    {
        AST_LITERAL_TYPE_STRING,
        AST_LITERAL_TYPE_INT,
    } type;

    union
    {
        u64 integer;
        char *str;
    };
};

#define AST_LITERAL_STRING(string)                                             \
    ((struct ast_literal){.type = AST_LITERAL_TYPE_STRING,                     \
                          .str = strdup(string)})
#define AST_LITERAL_INT(num)                                                   \
    ((struct ast_literal){.type = AST_LITERAL_TYPE_INT, .integer = num})

struct ast_value
{
    union
    {
        char *varname;
        struct ast_literal literal;
    };
    bool is_var;
};

#define AST_VALUE_VAR(_var)                                                    \
    ((struct ast_value){.is_var = true, .varname = _var})
#define AST_VALUE_LITERAL(_literal)                                            \
    ((struct ast_value){.is_var = false, .literal = _literal})

struct ast_function_arg
{
    struct ast_expr *expr;
    struct list list;
};

struct ast_function_call
{
    struct list arguments_head;
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
        struct
        {
            struct ast_expr *expr;
            char *operator;
        } unary_operator;
    };

    enum ast_expr_type
    {
        AST_EXPRESSION_VALUE,
        AST_EXPRESSION_CALL,
        AST_EXPRESSION_OPERATOR,
        AST_EXPRESSION_UNARY_OPERATOR,
    } type;
};
struct ast_expr *ast_parse_expression(struct ast_parser *parser,
                                      struct ast_error **err);

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
            struct list block; /* type is if, while, or for */
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

    struct list list;
};

/* function argument definition */
struct ast_function_arg_def
{
    struct ast_type *type;
    char *name;
    struct list list;
};

struct ast_function_declare
{
    struct ast_type *return_type;
    char *name;
    struct list args_head;

    struct list block;
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

    struct list list;
};

/* returns NULL if there was no error */
struct ast_error *ast_parse(struct list *tokens,
                            struct list *top_statements_head);

/* interpreter part, maybe move this later */

struct scope
{
    struct list list;
    struct array vars;
};

struct interpreter
{
    /* the current scopes the interpreter is in for one function, when a
     * function is called, the scope of that function will be stored on the
     * stack, to not interfere with the scope of the called function  */
    struct list *all_scopes;
    struct list *current_scope;
    struct array funcs; /* this should probably used a linked list, as it is
                           currently storing pointers */
    u8 *memory;
    size_t string_offset; /* TODO: string literals should probably be stored in
                             their own area */
};

static inline struct interpreter *interpreter_init()
{
    struct interpreter *out = malloc(sizeof(struct interpreter));
    out->funcs = ARRAY_INIT();
    out->all_scopes = malloc(sizeof(struct list));
    list_init_head(out->all_scopes);
    out->current_scope = out->all_scopes;
    out->memory = malloc(100);
    out->string_offset = 0;
    return out;
}

struct interp_value
{
    struct ast_type *type;
    u64 value;
};
struct interp_value *statements_eval(struct list *statements_head,
                                     struct interpreter *);
struct interp_value *expr_eval(struct ast_expr *expr, struct interpreter *);
int eval_program(struct list *top_statements_head);

#endif
