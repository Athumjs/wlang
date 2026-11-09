#include "lir.h"
#include "../backs.h"
#include <utils/numeric.h>
#include <stdlib.h>

void op_const(struct LIROperand *operand, struct IRValue *value, struct LIRContext *ctx) {
  if (value->kind == Value_Float || value->kind == Value_Double) {
    operand->kind = LIR_Float;
    operand->mem.base_reg = ctx->arch->inst_register;
    operand->mem.disp = (*ctx->floats_len)++;
    operand->mem.index_reg = -1;
    return;
  }

  operand->kind = LIR_Immediate;
  operand->imm = value->i;
}

void op_reg(struct LIROperand *operand, int64_t value, struct LIRContext *ctx) {
  struct Interval *interval = &ctx->live->intervals[value];

  if (interval->reg != -1) {
    operand->kind = LIR_Register;
    operand->reg = interval->reg;
    return;
  }

  operand->kind = LIR_Address;
  operand->mem.base_reg = ctx->arch->stack_base_register;
  operand->mem.disp = interval->spill_param ? interval->spillSlot : -interval->spillSlot;
  operand->mem.index_reg = -1;
}

void op_block(struct LIROperand *operand, int64_t value, struct LIRContext *ctx) {
  operand->kind = LIR_Label;
  int len = getNumCase(value) + 4;
  operand->label = malloc(len);
  snprintf(operand->label, len, ".Lb%ld", value);
}

void op_string(struct LIROperand *operand, struct String *str, struct LIRContext *ctx) {
  operand->kind = LIR_Label;
  int len = str->length + 1;
  operand->label = malloc(len);
  snprintf(operand->label, len, "%.*s", str->length, str->start);
}

void op_global(struct LIROperand *operand, struct String *str, struct LIRContext *ctx) {
  operand->kind = LIR_Global;
  int len = str->length + 1;
  operand->label = malloc(len);
  snprintf(operand->label, len, "%.*s", str->length, str->start);
}

void liroperand(struct LIROperand *operand, struct IROperand *iroperand, struct LIRContext *ctx) {
  operand->size = sizeOf(&iroperand->op_type);

  if (iroperand->kind == Operand_Constant)
    op_const(operand, &iroperand->constant, ctx);
  else if (iroperand->kind == Operand_Register)
    op_reg(operand, iroperand->ref, ctx);
  else if (iroperand->kind == Operand_Block)
    op_block(operand, iroperand->ref, ctx);
  else if (iroperand->kind == Operand_String)
    op_string(operand, iroperand->string, ctx);
  else if (iroperand->kind == Operand_Global)
    op_global(operand, iroperand->string, ctx);
}
