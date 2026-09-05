#ifndef CODEGEN_H
#define CODEGEN_H

#include <lib/compiler/c_compiler.h>
#include <types.h>

int generate_and_run_expr(struct ast_expr *expr);

#endif
