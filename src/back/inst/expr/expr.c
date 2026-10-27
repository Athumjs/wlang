#include "expr.h"

int resolveExpr_code(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->opcode == OPCODE_CONST) resolveConst_inst(regs, spill_slot, live, inst, file);
  else if (inst->opcode == OPCODE_ADD) resolveAdd_inst(regs, spill_slot, live, inst, file);
  else if (inst->opcode == OPCODE_SUB) resolveSub_inst(regs, spill_slot, live, inst, file);
  else if (inst->opcode == OPCODE_MUL) resolveMul_inst(regs, spill_slot, live, inst, file);
  else if (inst->opcode == OPCODE_TRUNC) resolveTrunc_inst(regs, live, inst, file);
  else if (inst->opcode == OPCODE_ZEXT) resolveZext_inst(regs, live, inst, file);
  else if (inst->opcode == OPCODE_SEXT) resolveSext_inst(regs, live, inst, file);
  else return 0;
  return 1;
}
