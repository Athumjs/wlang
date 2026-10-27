#include "expr.h"

void resolveConst_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  int reg = defineReg_code(regs, spill_slot, live, inst->vl, file);
  fprintf(file, "  mov %s, ", reg_names[reg][sizeOf(&inst->type)]);
  resolveOperand_code(live, &inst->operands[0], file);
  fprintf(file, "\n");
}
