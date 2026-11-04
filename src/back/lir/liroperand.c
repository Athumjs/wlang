#include "lir.h"

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
  operand->size = interval->size;

  if (interval->reg != -1) {
    operand->kind = LIR_Register;
    operand->reg = interval->reg;
    return;
  }

  operand->kind = LIR_Address;
  operand->mem.base_reg = ctx->arch->stack_register;
  operand->mem.disp = -interval->spillSlot;
  operand->mem.index_reg = -1;
}

void liroperand(struct LIROperand *operand, struct IROperand *iroperand, struct LIRContext *ctx) {
  if (iroperand->kind == Operand_Constant)
    op_const(operand, &iroperand->constant, ctx);
  else if (iroperand->kind == Operand_Register)
    op_reg(operand, iroperand->ref, ctx);
}
