#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <stdbool.h>

struct rect
{
    int x;
    int y;
    int w;
    int h;
};

static inline bool rect_intersect(struct rect one, struct rect two)
{
    return (one.x >= two.x && one.x + one.w <= two.x && one.y >= two.y &&
            one.y + one.h <= two.y);
}

#endif
