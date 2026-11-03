#include "x86_64_linux.h"

void x86_64_linux_operand(struct LIROperand *operand, FILE *file) {
  if (operand->kind == LIR_Immediate)
    fprintf(file, "$%ld", operand->imm);
  else if (operand->kind == LIR_Register)
    fprintf(file, "%%%s", reg_names[operand->reg][operand->size]);

  else if (operand->kind == LIR_Address) {
    if (operand->mem.disp > 0)
      fprintf(file, "%ld", operand->mem.disp);
    if (operand->mem.base_reg == 100) fprintf(file, "(%%rip)");
    else fprintf(file, "(%%%s)", reg_names[operand->mem.base_reg][operand->size]);
  }
}
