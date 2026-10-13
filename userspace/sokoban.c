#include "wm_lib.h"
#include <compiler.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdbool.h>
#include <string.h>

#define SOKOBAN_ROOT "/sokoban/"

struct wm_client client;

enum tile_state
{
    TILE_WALL = 1,
    TILE_GOAL = 3,
    TILE_BOX = 2,
    TILE_BOX_GOAL = 4,
    TILE_EMPTY = 0,
};

struct undo_entry
{
    int player_x, player_y;

    /* only used if undoing a box movement */
    enum tile_state prev_state;
    enum tile_state new_tile;
    int box_x, box_y;
    int new_box_x, new_box_y;

    struct list list;
};

LIST_DEFINE_HEAD(undo_queue);

struct undo_entry *undo_entry_alloc(int player_x, int player_y)
{
    struct undo_entry *out = malloc(sizeof(struct undo_entry));
    out->player_x = player_x;
    out->player_y = player_y;

    out->prev_state = -1;

    out->list = LIST_INIT();

    return out;
}

enum tile_state **parse_level(const char *level, size_t *w, size_t *h,
                              int *player_x, int *player_y)
{
    struct array rows = ARRAY_INIT();
    struct array current_row = ARRAY_INIT();
    size_t max_len = 0;

    size_t i = 0;
    size_t x = 0, y = 0;
    while (level[i] != 0)
    {

        enum tile_state state;
        switch (level[i])
        {
        case '#':
            state = TILE_WALL;
            break;
        case '$':
            state = TILE_BOX;
            break;
        case '@':
            *player_x = x;
            *player_y = y;
            state = TILE_EMPTY;
            break;
        case '.':
            state = TILE_GOAL;
            break;
        case ' ':
            state = TILE_EMPTY;
            break;
        }

        if (level[i] == '\n')
        {
            size_t len = ARRAY_LEN(current_row, sizeof(enum tile_state));
            if (len > max_len)
            {
                max_len = len;
            }

            y++;
            x = 0;

            array_add(&rows, &current_row, sizeof(struct array));

            current_row = ARRAY_INIT();
        }
        else
        {
            array_add(&current_row, &state, sizeof(enum tile_state));
            x++;
        }

        i++;
    }

    *w = max_len;
    *h = ARRAY_LEN(rows, sizeof(struct array));

    struct array out = ARRAY_INIT(); /* stupid to allocate two arrays */
    for (size_t j = 0; j < ARRAY_LEN(rows, sizeof(struct array)); j++)
    {
        struct array row_array =
            *((struct array *)array_get(&rows, j, sizeof(struct array)));

        size_t len = ARRAY_LEN(row_array, sizeof(enum tile_state));

        if (len < max_len)
        {
            for (size_t i = 0; i < max_len - len; i++)
            {
                enum tile_state padding = TILE_EMPTY;
                array_add(&row_array, &padding, sizeof(enum tile_state));
            }
        }

        enum tile_state *ptr = array_free_ptr(row_array);
        array_add(&out, &ptr, sizeof(enum tile_state *));
    }

    return array_free_ptr(out);
}

enum tile_state static_game[][11] = {
    // clang-format off
        {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 2, 0, 0, 0, 2, 0, 2, 0, 1},
        {1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1},
        {1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 1},
        {1, 0, 3, 3, 0, 0, 2, 1, 2, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 3, 3, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    // clang-format on
};

enum tile_state **game = NULL;

#define WINDOW_W 600
#define WINDOW_H 600
#define TILE_IMG_SIZE 60

size_t num_goal_spaces = 0;
int num_goal_boxes = 0;

void update_goal_boxes(int new_val)
{
    num_goal_boxes = new_val;
    if (num_goal_boxes == num_goal_spaces)
    {
        logi("you win!");
    }
    if (num_goal_spaces < 0)
    {
        PANIC("number of correct goal spaces was less than 0");
    }
}

struct image player_img;
struct image check_img;
struct image crate_img;
struct image goal_img;
struct image wall_img;
struct image floor_img;

int player_x = 0, player_y = 0;

size_t game_w = 0, game_h = 0;

bool should_update_wm = false;
size_t scale = 45;
double img_scale = 0.75;
void draw_player(size_t x, size_t y)
{
    framebuffer_draw_img_scale(&client.fb, x * scale, y * scale, img_scale,
                               player_img);
    if (likely(should_update_wm))
    {
        wm_render_partial(client,
                          (struct rect){x * scale, y * scale,
                                        player_img.header.w * img_scale,
                                        player_img.header.h * img_scale});
    }
}

void draw_tile(size_t x, size_t y)
{
    enum tile_state state = game[y][x];
    struct image img;
    switch (state)
    {
    case TILE_WALL:
        img = wall_img;
        break;
    case TILE_GOAL:
        img = goal_img;
        break;
    case TILE_BOX:
        img = crate_img;
        break;
    case TILE_BOX_GOAL:
        img = check_img;
        break;
    case TILE_EMPTY:
        img = floor_img;
        break;
    }

    framebuffer_draw_img_scale(&client.fb, x * scale, y * scale, img_scale,
                               img);
    if (likely(should_update_wm))
    {
        wm_render_partial(client, (struct rect){x * scale, y * scale,
                                                img.header.w * img_scale,
                                                img.header.h * img_scale});
    }
}

static inline void clamp_coordinate(int *x, int *y)
{
    if (unlikely(*x < 0))
    {
        *x = 0;
    }
    if (unlikely(*x > game_w - 1))
    {
        *x = game_w - 1;
    }

    if (unlikely(*y < 0))
    {
        *y = 0;
    }
    if (unlikely(*y > game_h - 1))
    {
        *y = game_h - 1;
    }
}

void move_player(int x, int y)
{

    int old_x = player_x, old_y = player_y;

    int new_x = player_x + x;
    int new_y = player_y + y;

    clamp_coordinate(&new_x, &new_y);

    enum tile_state state = game[new_y][new_x];

    if (state == TILE_EMPTY || state == TILE_GOAL)
    {
        player_x = new_x;
        player_y = new_y;

        draw_tile(old_x, old_y);
        draw_player(player_x, player_y);
        struct undo_entry *ent = undo_entry_alloc(old_x, old_y);
        list_push_end(&ent->list, &undo_queue);
    }

    if (state == TILE_BOX || state == TILE_BOX_GOAL)
    {
        int box_x = new_x + x;
        int box_y = new_y + y;

        clamp_coordinate(&box_x, &box_y);

        enum tile_state new_box_location = game[box_y][box_x];
        if (new_box_location == TILE_EMPTY || new_box_location == TILE_GOAL)
        {

            struct undo_entry *ent = undo_entry_alloc(old_x, old_y);
            ent->new_box_x = box_x;
            ent->new_box_y = box_y;

            ent->prev_state = state;
            ent->box_x = new_x;
            ent->box_y = new_y;
            ent->new_tile = new_box_location;
            list_push_end(&ent->list, &undo_queue);

            if (new_box_location == TILE_EMPTY)
            {
                if (state == TILE_BOX_GOAL)
                {
                    update_goal_boxes(num_goal_boxes - 1);
                }
                game[box_y][box_x] = TILE_BOX;
            }
            else if (new_box_location == TILE_GOAL)
            {
                if (state == TILE_BOX)
                {
                    update_goal_boxes(num_goal_boxes + 1);
                }
                game[box_y][box_x] = TILE_BOX_GOAL;
            }

            if (state == TILE_BOX)
            {
                game[new_y][new_x] = TILE_EMPTY;
            }
            else if (state == TILE_BOX_GOAL)
            {
                game[new_y][new_x] = TILE_GOAL;
            }

            draw_tile(box_x, box_y);

            player_x = new_x;
            player_y = new_y;
            draw_tile(old_x, old_y);
            draw_player(player_x, player_y);
        }
    }
}

void undo()
{
    struct list *list = list_pop_end(&undo_queue);
    if (list == NULL)
    {
        return;
    }

    struct undo_entry *ent = containerof(list, struct undo_entry, list);
    draw_tile(player_x, player_y);
    player_x = ent->player_x;
    player_y = ent->player_y;
    draw_player(player_x, player_y);
    if (ent->prev_state != -1)
    {
        game[ent->box_y][ent->box_x] = ent->prev_state;
        game[ent->new_box_y][ent->new_box_x] = ent->new_tile;

        if (ent->prev_state == TILE_BOX_GOAL &&
            ent->new_tile == TILE_EMPTY) /* moving from empty to goal */
        {
            update_goal_boxes(num_goal_boxes + 1);
        }

        if (ent->prev_state == TILE_BOX &&
            ent->new_tile == TILE_GOAL) /* moving from goal to empty */
        {
            update_goal_boxes(num_goal_boxes - 1);
        }

        draw_tile(ent->box_x, ent->box_y);
        draw_tile(ent->new_box_x, ent->new_box_y);
    }

    free(ent);
}

void load_level(const char *path)
{
    char *str = malloc(strlen(path) + strlen(SOKOBAN_ROOT));
    stpcpy(stpcpy(str, SOKOBAN_ROOT), path);

    int fd = open(str, O_RDONLY);

    if (fd == -1)
    {
        return;
    }
    size_t read_size = 10;
    size_t buf_len = read_size;

    char *buf = malloc(buf_len);
    size_t buf_offset = 0;
    size_t bytes_read;
    while ((bytes_read = read(fd, buf + buf_offset, read_size)) >= read_size)
    {
        buf_len += read_size;
        buf_offset += read_size;
        buf = realloc(buf, buf_len);
    }
    game = parse_level(buf, &game_w, &game_h, &player_x, &player_y);

    double scale_factor = (double)WINDOW_W / game_w;
    img_scale = (double)scale_factor / TILE_IMG_SIZE;
    scale = scale_factor;

    logd("%f %f %d", img_scale, scale_factor, scale);

    for (size_t y = 0; y < game_h; y++)
    {
        for (size_t x = 0; x < game_w; x++)
        {

            if (game[y][x] == TILE_GOAL)
            {
                num_goal_spaces++;
            }
            draw_tile(x, y);
        }
    }
    draw_player(player_x, player_y);

    free(buf);
    close(fd);
}

int main()
{
    image_parse(SOKOBAN_ROOT "images/player.tga", &player_img);
    image_parse(SOKOBAN_ROOT "images/box_goal.tga", &check_img);
    image_parse(SOKOBAN_ROOT "images/goal.tga", &goal_img);
    image_parse(SOKOBAN_ROOT "images/box.tga", &crate_img);
    image_parse(SOKOBAN_ROOT "images/wall.tga", &wall_img);
    image_parse(SOKOBAN_ROOT "images/floor.tga", &floor_img);

    client = wm_client_connect(WINDOW_H, WINDOW_W);
    load_level("level2");

    wm_render(client);
    should_update_wm = true;
    bool reading_level = false;

    struct array levelname = ARRAY_INIT();

    while (true)
    {
        struct wm_message message;
        wm_get_message(&client, &message);

        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:;
            char c = *(char *)message.message_data;
            if (reading_level)
            {
                if (c == '\n')
                {
                    char terminator = 0;
                    array_add(&levelname, &terminator, 1);
                    load_level(levelname.data);
                    array_free(&levelname);
                    reading_level = false;
                    levelname = ARRAY_INIT();
                }
                else if (c == '\b')
                {
                    array_pop_end(&levelname, 1);
                    log_multiline_start(&LOG_DEBUG);
                    for (size_t i = 0; i < levelname.len; i++)
                    {
                        logd("%c", ((char *)levelname.data)[i]);
                    }
                    log_multiline_end(&LOG_DEBUG);
                }
                else
                {
                    array_add(&levelname, &c, 1);
                    log_multiline_start(&LOG_DEBUG);
                    for (size_t i = 0; i < levelname.len; i++)
                    {
                        logd("%c", ((char *)levelname.data)[i]);
                    }
                    log_multiline_end(&LOG_DEBUG);
                }
            }
            else
            {
                switch (c)
                {
                case 'h':
                    move_player(-1, 0);
                    break;
                case 'l':
                    move_player(1, 0);
                    break;
                case 'j':
                    move_player(0, 1);
                    break;
                case 'k':
                    move_player(0, -1);
                    break;
                case 'u':
                    undo();
                    break;
                case 0x1B: /* escape */
                    reading_level = true;
                    break;
                }
            }
            break;
        }
    }

    exit(0);
}
