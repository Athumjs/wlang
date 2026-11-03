#include <middle/irgen.h>
#include "ir.h"

void loweringAST(struct Program *program, struct IRModule *ir, uint8_t show_mir) {
  for (int i = 0; i < program->length; i++) irGlobal(program->decls[i], ir, program->arena);
  if (show_mir) showIR(ir);
}
