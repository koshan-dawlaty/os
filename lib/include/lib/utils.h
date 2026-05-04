#ifndef LIB_UTILS_H
#define LIB_UTILS_H

#define MACRO_STR2(x) #x
#define MACRO_STR(x) MACRO_STR2(x)

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define ABS(a) ((a < 0) ? -a : a)

#endif
