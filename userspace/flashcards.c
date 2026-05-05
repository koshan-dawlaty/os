#include "wm_lib.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

struct PSF_font *font;
struct wm_client client;

struct set_entry
{
    char *question;
    char *answer;
    struct list2c list;
};

void parse_flashcards(char *buf, struct list2c *head)
{
    char *tok = strtok(buf, "\n");

    struct list2c *current_list = head;
    struct set_entry *current_entry = NULL;
    bool is_question = true;

    while (tok != NULL)
    {
        if (*tok == 0)
        {
            goto end; /* line was only a newline */
        }
        if (is_question)
        {
            struct set_entry *new_entry = malloc(sizeof(struct set_entry));
            new_entry->list = LIST2C_INIT();
            list2c_add_after(&new_entry->list, current_list);
            current_entry = new_entry;
            current_list = &new_entry->list;

            current_entry->question = strdup(tok);
            is_question = false;
        }
        else
        {
            current_entry->answer = strdup(tok);
            is_question = true;
        }
    end:
        tok = strtok(NULL, "\n");
    }
}

void display_entry(struct set_entry *entry)
{
    client_draw_rect(client, 0, 0,
                     MAX(strlen(entry->question), strlen(entry->answer)) *
                         font->header.width,
                     font->header.height * 2, 0);
    client_draw_string(client, font, entry->question, 0, 0);
    client_draw_string(client, font, entry->answer, 0, font->header.height);
    wm_render(client);
}

void free_entries(struct list2c *head)
{
    struct set_entry *prev = NULL;
    struct list2c *pos;
    list2c_for_each(pos, head)
    {
        struct set_entry *entry = containerof(pos, struct set_entry, list);
        if (prev != NULL)
        {
            free(prev->question);
            free(prev->answer);
            free(prev);
        }
        prev = entry;
    }
    if (prev != NULL)
    {
        free(prev->question);
        free(prev->answer);
        free(prev);
    }
}

#define OUT_OF_ENTRIES_STR "out of entries"

int main(int argc, char **argv)
{

    init_libc();

    client = wm_client_connect(600, 200);
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    wm_render(client);

    int fd = open("/flashcards", O_RDONLY);
    size_t read_size = 10;
    size_t result = 0;
    char *flashcards = malloc(read_size);
    size_t flashcards_offset = 0;
    while ((result = read(fd, flashcards + flashcards_offset, read_size)) >=
           read_size)
    {
        if (result < 0)
        {
            PANIC("invalid result returned by read");
        }
        flashcards_offset += read_size;
        flashcards = realloc(flashcards, flashcards_offset + read_size);
    }
    flashcards[flashcards_offset + result] = 0;
    close(fd);

    LIST2C_DEFINE_HEAD(head);

    parse_flashcards(flashcards, &head);

    struct list2c *current = head.next;

    while (1)
    {
        if (current == &head)
        {

            client_draw_rect(client, 0, 0,
                             strlen(OUT_OF_ENTRIES_STR) * font->header.width,
                             font->header.height * 2, 0);
            client_draw_string(client, font, OUT_OF_ENTRIES_STR, 0, 0);
            wm_render(client);
            wm_get_key(client);
            break;
        }
        struct set_entry *entry = containerof(current, struct set_entry, list);
        current = current->next;
        display_entry(entry);
        wm_get_key(client);
    }
    free_entries(&head);

    close(client.socket);

    exit(0);
}
