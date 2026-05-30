#include "container_of.h"
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

};

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
    return (c == ' ');
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

struct token *get_token(const char *input, size_t *len)
{

    while (is_space(input[*len]))
    {
        (*len)++;
    }

    const char *s = input + *len;
    logd("|%s|", s);

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

    if (s[0] == ';')
    {
        return token_alloc_literal(TOKEN_SEMICOLON, ";", len);
    }

    return token_alloc(TOKEN_UNKNOWN, "", 0);
}

int main()
{
    init_libc();

    const char *program = "int x  =10;";
    size_t offset = 0;

    LIST2C_DEFINE_HEAD(token_list);

    size_t len = 0;
    struct token *t = get_token(program, &len);
    offset += len;
    while (t->type != TOKEN_EOF && t->type != TOKEN_UNKNOWN)
    {
        list2c_push_end(&t->list, &token_list); /* NEW FUNCTION CHECK THIS */

        size_t len = 0;
        t = get_token(program + offset, &len);
        offset += len;
    }

    struct list2c *pos;
    list2c_for_each(pos, &token_list)
    {
        struct token *tok = containerof(pos, struct token, list);
        if (tok->type == TOKEN_UNKNOWN)
        {
            logd("i got this unknown token:%s", tok->token);
        }
        else
        {
            logd("i got this token:%s", tok->token);
        }
    }

    exit(0);
    return 0;
}
