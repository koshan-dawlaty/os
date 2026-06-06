#include "compiler.h"
#include "lib/framebuffer.h"
#include "lib/img.h"
#include "lib/log.h"
#include "wm_lib.h"
#include <init.h>
#include <math.h>
#include <stdbool.h>

struct wm_client client;
size_t scale = 30;

enum tile_state
{
    TILE_WALL = 1,
    TILE_GOAL = 3,
    TILE_BOX = 2,
    TILE_BOX_GOAL = 4,
    TILE_EMPTY = 0,
};

enum tile_state game[][20] = {
    // clang-format off
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 2, 0, 3, 3, 1},
        {1, 0, 0, 0, 1, 1, 1, 1, 3, 3, 1},
        {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    // clang-format on
};

struct image player_img;
struct image check_img;
struct image crate_img;
struct image goal_img;
struct image wall_img;
struct image floor_img;

int player_x = 0, player_y = 0;

size_t game_w = sizeof(game[0]) / sizeof(game[0][0]);
size_t game_h = sizeof(game) / sizeof(game[0]);

void draw_player(size_t x, size_t y)
{
    framebuffer_draw_img(&client.fb, x * scale, y * scale, player_img);
    /* framebuffer_draw_rect(&client.fb, x * scale, y * scale, scale, scale, */
    /*                       0x00FF00); */
}

void draw_tile(size_t x, size_t y)
{
    enum tile_state state = game[y][x];
    switch (state)
    {
    case TILE_WALL:
        framebuffer_draw_img(&client.fb, x * scale, y * scale, wall_img);
        break;
    case TILE_GOAL:
        framebuffer_draw_img(&client.fb, x * scale, y * scale, goal_img);
        break;
    case TILE_BOX:
        framebuffer_draw_img(&client.fb, x * scale, y * scale, crate_img);
        break;
    case TILE_BOX_GOAL:
        framebuffer_draw_img(&client.fb, x * scale, y * scale, check_img);
        break;
    case TILE_EMPTY:
        framebuffer_draw_img(&client.fb, x * scale, y * scale, floor_img);

        break;
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
    }

    if (state == TILE_BOX || state == TILE_BOX_GOAL)
    {
        int box_x = new_x + x;
        int box_y = new_y + y;

        clamp_coordinate(&box_x, &box_y);

        enum tile_state new_box_location = game[box_y][box_x];
        if (new_box_location == TILE_EMPTY || new_box_location == TILE_GOAL)
        {

            if (new_box_location == TILE_EMPTY)
            {
                game[box_y][box_x] = TILE_BOX;
            }
            else if (new_box_location == TILE_GOAL)
            {
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

int main()
{
    init_libc();
    image_parse("/images/dog123.tga", &player_img);
    image_parse("/images/check.tga", &check_img);
    image_parse("/images/goal.tga", &goal_img);
    image_parse("/images/crate.tga", &crate_img);
    image_parse("/images/wall.tga", &wall_img);
    image_parse("/images/grass.tga", &floor_img);

    client = wm_client_connect(game_h * scale, game_w * scale);

    for (size_t y = 0; y < game_h; y++)
    {
        for (size_t x = 0; x < game_w; x++)
        {
            draw_tile(x, y);
        }
    }

    wm_render(client);

    while (true)
    {
        struct wm_message message;
        wm_get_message(client, &message);

        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:;
            char c = *(char *)message.message_data;
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
            }
            break;
        }
        wm_render(client);
    }

    exit(0);
}
