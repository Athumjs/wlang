#include "expr.h"

void resolveAdd_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->operands[0].kind == Operand_Register) {
    regs[live->intervals[inst->operands[0].ref].phys] = inst->vl;
    live->intervals[inst->vl].phys = live->intervals[inst->operands[0].ref].phys;
    fprintf(file, "  add ");
    resolveOperand_code(live, &inst->operands[0], file);
  } else {
    int reg = defineReg_code(regs, spill_slot, live, inst->vl, file);
    fprintf(file, "  mov %s, ", reg_names[reg][sizeOf(&inst->type)]);
    resolveOperand_code(live, &inst->operands[0], file);
    fprintf(file, "\n  add %s", reg_names[reg][sizeOf(&inst->type)]);
  }

  fprintf(file, ", ");
  resolveOperand_code(live, &inst->operands[1], file);
  fprintf(file, "\n");
}

void resolveSub_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->operands[0].kind == Operand_Register) {
    regs[live->intervals[inst->operands[0].ref].phys] = inst->vl;
    live->intervals[inst->vl].phys = live->intervals[inst->operands[0].ref].phys;
    fprintf(file, "  sub ");
    resolveOperand_code(live, &inst->operands[0], file);
  } else {
    int reg = defineReg_code(regs, spill_slot, live, inst->vl, file);
    fprintf(file, "  mov %s, ", reg_names[reg][sizeOf(&inst->type)]);
    resolveOperand_code(live, &inst->operands[0], file);
    fprintf(file, "\n  sub %s", reg_names[reg][sizeOf(&inst->type)]);
  }

  fprintf(file, ", ");
  resolveOperand_code(live, &inst->operands[1], file);
  fprintf(file, "\n");
}

void resolveMul_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->operands[0].kind == Operand_Register) {
    regs[live->intervals[inst->operands[0].ref].phys] = inst->vl;
    live->intervals[inst->vl].phys = live->intervals[inst->operands[0].ref].phys;
    fprintf(file, "  imul ");
    resolveOperand_code(live, &inst->operands[0], file);
  } else {
    int reg = defineReg_code(regs, spill_slot, live, inst->vl, file);
    fprintf(file, "  mov %s, ", reg_names[reg][sizeOf(&inst->type)]);
    resolveOperand_code(live, &inst->operands[0], file);
    fprintf(file, "\n  imul %s", reg_names[reg][sizeOf(&inst->type)]);
  }

  fprintf(file, ", ");
  resolveOperand_code(live, &inst->operands[1], file);
  fprintf(file, "\n");
}
