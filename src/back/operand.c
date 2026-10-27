#include "back.h"

void resolveValue_code(struct IRValue *value, FILE *file) {
  if (value->kind == Value_Int)
    fprintf(file, "%ld", value->i);
  else if (value->kind == Value_UInt || value->kind == Value_Char)
    fprintf(file, "%lu", value->u);
  else if (value->kind == Value_Float)
    fprintf(file, "%f", value->f);
  else if (value->kind == Value_Double)
    fprintf(file, "%lf", value->d);
  else if (value->kind == Value_String)
    fprintf(file, "\"%.*s\"", value->s->length, value->s->start);

  else if (value->kind == Value_Array) {
    for (int i = 0; i < value->a.elems_len; i++) {
      if (i > 0) fprintf(file, ", ");
      resolveValue_code(&value->a.elems[i], file);
    }
  }
}

void resolveReg_code(struct Live *live, int64_t value, FILE *file) {
  int reg = live->intervals[value].phys;
  int size = live->intervals[value].size;

  if (reg >= 0 && reg < 14)
    fprintf(file, "%s", reg_names[reg][size]);
  else fprintf(file, "[rsp + %d]", live->intervals[value].spillSlot);
}

void resolveOperand_code(struct Live *live, struct IROperand *operand, FILE *file) {
  if (operand->kind == Operand_Constant)
    resolveValue_code(&operand->constant, file);
  else if (operand->kind == Operand_Register)
    resolveReg_code(live, operand->ref, file);
}
