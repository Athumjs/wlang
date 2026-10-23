#include <middle/irgen.h>
#include "ir.h"

void loweringAST(struct Program *program, struct IRModule *ir) {
  for (int i = 0; i < program->length; i++) irGlobal(program->decls[i], ir, program->arena);
}
