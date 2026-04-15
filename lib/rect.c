#include <lib/log.h>
#include <lib/rect.h>
#include <types.h>

u8 rect_split_intersect(struct rect one, struct rect two, struct rect *result)
{
    struct rect intersect = rect_get_intersect(one, two);
    if (rect_eq(intersect, RECT_INIT(0, 0, 0, 0)))
    {
        return 0;
    }

    u8 retval = 0;

    if (intersect.y > one.y)
    {
        result[retval] = RECT_INIT(one.x, one.y, one.w, intersect.y - one.y);
        retval++;
    }

    if (intersect.y + intersect.h < one.y + one.h)
    {
        result[retval] =
            RECT_INIT(one.x, intersect.y + intersect.h, one.w,
                      (one.y + one.h) - (intersect.y + intersect.h));
        retval++;
    }

    if (intersect.x > one.x)
    {
        result[retval] =
            RECT_INIT(one.x, intersect.y, intersect.x - one.x, intersect.h);
        retval++;
    }

    if (intersect.x + intersect.w < one.x + one.w)
    {
        result[retval] = RECT_INIT(
            intersect.x + intersect.w, intersect.y,
            (one.x + one.w) - (intersect.x + intersect.w), intersect.h);
        retval++;
    }

    return retval;
}
