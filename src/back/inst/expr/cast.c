#include "expr.h"

void resolveTrunc_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file) {
  size_t size = sizeOf(&inst->operands[1].type);
  live->intervals[inst->vl].size = size;

  if (sizeOf(&inst->type) <= 2 || size <= 2)
    size = 2; 

  live->intervals[inst->operands[0].ref].size = size;
  fprintf(file, "  mov "); 

  for (int i = 0; i < 14; i++) {
    if (regs[i] != -1) continue;
    regs[i] = inst->vl;
    live->intervals[inst->vl].phys = i;
    fprintf(file, "%s, ", reg_names[i][size]);
    break;
  }

  resolveOperand_code(live, &inst->operands[0], file);
  fprintf(file, "\n");
}

void resolveZext_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file) {
  live->intervals[inst->vl].size = sizeOf(&inst->operands[1].type);
  fprintf(file, "  movsx ");

  for (int i = 0; i < 14; i++) {
    if (regs[i] != -1) continue;
    regs[i] = inst->vl;
    live->intervals[inst->vl].phys = i;
    fprintf(file, "%s, ", reg_names[i][live->intervals[inst->vl].size]);
    break;
  }

  resolveOperand_code(live, &inst->operands[0], file);
  fprintf(file, "\n");
}

void resolveSext_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file) {
  live->intervals[inst->vl].size = sizeOf(&inst->operands[1].type);
  fprintf(file, "  movsx ");

  for (int i = 0; i < 14; i++) {
    if (regs[i] != -1) continue;
    regs[i] = inst->vl;
    live->intervals[inst->vl].phys = i;
    fprintf(file, "%s, ", reg_names[i][live->intervals[inst->vl].size]);
    break;
  }

  resolveOperand_code(live, &inst->operands[0], file);
  fprintf(file, "\n");
}
