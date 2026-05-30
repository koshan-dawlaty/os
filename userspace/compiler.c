#include "container_of.h"
#include "lib/assert.h"
#include "lib/font.h"
#include <init.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_type
{
    TOKEN_IDENTIFIER,
    TOKEN_EOF,
    TOKEN_LITERAL,
    TOKEN_UNKNOWN,
    TOKEN_ASSIGN,
    TOKEN_EQUALS,
    TOKEN_INT,
    TOKEN_SEMICOLON,
    TOKEN_O_PAREN,
    TOKEN_C_PAREN,
    TOKEN_O_BRACE,
    TOKEN_C_BRACE,

};

const char *token_type_str(enum token_type type)
{
    switch (type)
    {
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_LITERAL:
        return "LITERAL";
    case TOKEN_UNKNOWN:
        return "UNKNOWN";
    case TOKEN_ASSIGN:
        return "ASSIGN";
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

    struct token *simple = get_token_simple(s, len);
    if (simple != NULL)
    {
        return simple;
    }

    return token_alloc(TOKEN_UNKNOWN, "", 0);
}

struct tokenizer_test_input
{
    const char *str;
    enum token_type type;
};

void tokenizer_test_impl(const char *program,
                         struct tokenizer_test_input expected[],
                         size_t expected_len)
{

    size_t offset = 0;

    LIST2C_DEFINE_HEAD(token_list);

    size_t len = 0;
    struct token *t = get_token(program, &len);
    offset += len;
    while (t->type != TOKEN_EOF && t->type != TOKEN_UNKNOWN)
    {
        list2c_push_end(&t->list, &token_list);

        size_t len = 0;
        t = get_token(program + offset, &len);
        offset += len;
    }

    size_t i = 0;
    struct list2c *pos;
    list2c_for_each(pos, &token_list)
    {
        if (i >= expected_len)
        {
            PANIC("more tokens than expected in output");
        }
        struct token *tok = containerof(pos, struct token, list);
        if (tok->type == TOKEN_UNKNOWN)
        {
            PANIC("unknown token:%s", tok->token);
        }

        if (expected[i].type != tok->type)
        {
            PANIC("expected token of type:%s, got %s",
                  token_type_str(expected->type), token_type_str(tok->type));
        }

        if (strcmp(expected[i].str, tok->token))
        {
            PANIC("expected token of value:|%s|, got |%s|", expected[i].str,
                  tok->token);
        }

        i++;
    }

    if (i != expected_len)
    {
        PANIC("less tokens than expected in output");
    }
}

#define tokenizer_test(program, ...)                                           \
    tokenizer_test_impl(                                                       \
        program, ((struct tokenizer_test_input[]){__VA_ARGS__}),               \
        sizeof(((struct tokenizer_test_input[]){__VA_ARGS__})) /               \
            sizeof(struct tokenizer_test_input))

void tokenizer_tests()
{
    // clang-format off
    tokenizer_test("int   x   =   10   ;    ",
                   {"int", TOKEN_IDENTIFIER},
				   {"x", TOKEN_IDENTIFIER},
                   {"=", TOKEN_ASSIGN},
				   {"10", TOKEN_INT},
                   {";", TOKEN_SEMICOLON}
				   );


    tokenizer_test("int test123=10; int x = 123;",
                   {"int", TOKEN_IDENTIFIER},
				   {"test123", TOKEN_IDENTIFIER},
                   {"=", TOKEN_ASSIGN},
				   {"10", TOKEN_INT},
                   {";", TOKEN_SEMICOLON},

                   {"int", TOKEN_IDENTIFIER},
				   {"x", TOKEN_IDENTIFIER},
                   {"=", TOKEN_ASSIGN},
				   {"123", TOKEN_INT},
                   {";", TOKEN_SEMICOLON}
				   );

    tokenizer_test(
			"void main(int argc) {"
			"	print(123);"
			"}"
			,
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
                   {"}", TOKEN_C_BRACE}

				   );
    // clang-format on
}

int main()
{
    init_libc();

    tokenizer_tests();

    exit(0);

    return 0;
}
