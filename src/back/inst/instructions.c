#include "inst.h"

int defineReg_code(int64_t *regs, int *spill_slot, struct Live *live, int64_t value, FILE *file) {
  for (int i = 0; i < 14; i++) {
    if (regs[i] != -1) continue;
    regs[i] = value;
    live->intervals[value].phys = i;
    return i;
  }

  live->intervals[value].phys = -1;
  *spill_slot += 8;
  live->intervals[value].spillSlot = *spill_slot;
  return -1;
}

void resolveInstructions_code(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->vl != -1) {
    live->intervals[inst->vl].size = sizeOf(&inst->type);
    for (int i = 0; i < 14; i++) {
      if (regs[i] == -1) continue;
      if (live->intervals[inst->vl].start <= live->intervals[regs[i]].end) continue;
      regs[i] = -1;
    }
  } 

  int result = resolveStmt_code(regs, live, inst, file);
  if (!result) resolveExpr_code(regs, spill_slot, live, inst, file);
}
