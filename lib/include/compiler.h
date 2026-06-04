#ifndef COMPILER_H
#define COMPILER_H

#define __constructor __attribute__((constructor))
#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __section(name) __attribute__((section(name)))
#define __no_ssp __attribute__((no_stack_protector))

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#endif
