#include "x86_64_linux.h"

void x86_64_linux_operand(struct LIROperand *operand, FILE *file) {
  if (operand->kind == LIR_Immediate)
    fprintf(file, "$%ld", operand->imm);
  else if (operand->kind == LIR_Register)
    fprintf(file, "%%%s", reg_names[operand->reg][operand->size]);

  else if (operand->kind == LIR_Address || operand->kind == LIR_Float) {
    if (operand->mem.disp != 0) {
      if (operand->kind == LIR_Float)
        fprintf(file, "float_%ld", operand->mem.disp);
      else fprintf(file, "%ld", operand->mem.disp);
    }

    if (operand->mem.base_reg == 100) fprintf(file, "(%%rip");
    else fprintf(file, "(%%%s", reg_names[operand->mem.base_reg][operand->size]);

    if (operand->mem.index_reg != -1) {
      fprintf(file, ", %%%s", reg_names[operand->mem.index_reg][operand->size]);

      if (operand->mem.scale > 1)
        fprintf(file, ", %d", operand->mem.scale);
    }

    fprintf(file, ")");
  }
}
