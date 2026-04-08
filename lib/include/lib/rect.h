#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <lib/log.h>
#include <lib/utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <types.h>

struct rect
{
    int x;
    int y;
    int w;
    int h;
};

#define RECT_INIT(_x, _y, _w, _h)                                              \
    (struct rect)                                                              \
    {                                                                          \
        .x = _x, .y = _y, .w = _w, .h = _h                                     \
    }

static inline bool rect_intersect(struct rect one, struct rect two)
{

    size_t x1 = MAX(one.x, two.x);
    size_t y1 = MAX(one.y, two.y);

    size_t x2 = MIN(one.x + one.w, two.x + two.w);
    size_t y2 = MIN(one.y + one.h, two.y + two.h);

    return !(x1 > x2 || y1 > y2);
}

static inline struct rect rect_get_intersect(struct rect one, struct rect two)
{

    size_t x1 = MAX(one.x, two.x);
    size_t y1 = MAX(one.y, two.y);

    size_t x2 = MIN(one.x + one.w, two.x + two.w);
    size_t y2 = MIN(one.y + one.h, two.y + two.h);

    if (x1 > x2 || y1 > y2)
    {
        return RECT_INIT(0, 0, 0, 0);
    }

    return RECT_INIT(x1, y1, x2 - x1, y2 - y1);
}

static inline bool rect_eq(struct rect one, struct rect two)
{
    return one.x == two.x && one.y == two.y && one.h == two.h && one.w == two.w;
}

/*
 * gets rectangles that do not intersect @two, but are around it, @result should
 * be able to hold 4 values
 * returns number of values placed in result, how many rectanges there were
 */
u8 rect_split_intersect(struct rect one, struct rect two, struct rect *result);

#endif
