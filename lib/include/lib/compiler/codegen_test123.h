#ifndef CODEGEN_TEST123
#define CODEGEN_TEST123
/* for custom cpu */

/* FIXME: this is only for thing like bytecode_array and codegen_label, the
 * x86-64 stuff should be factored out */
#include "codegen.h"


void test123_generate_ir(struct bytecode_array *code,
                         struct ir_program *program);

#endif
