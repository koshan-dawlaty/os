#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <lib/utils.h>
#include <stdbool.h>
#include <stddef.h>

struct rect
{
    int x;
    int y;
    int w;
    int h;
};

static inline bool rect_intersect(struct rect one, struct rect two)
{

    size_t x1 = MAX(one.x, two.x);
    size_t y1 = MAX(one.y, two.y);

    size_t x2 = MIN(one.x + one.w, two.x + two.w);
    size_t y2 = MIN(one.y + one.h, two.y + two.h);

    return !(x1 > x2 || y1 > y2);
}

#endif
