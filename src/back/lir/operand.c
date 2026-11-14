#include "lir.h"
#include <utils/numeric.h>
#include <stdlib.h>

void op_const(struct LIRInstruction *inst, struct IRValue *value, struct IRType *op_type, struct LIRContext *ctx) {
  if (value->kind == Value_Float || value->kind == Value_Double)
    lir_addOperandAddress(1, ctx->arch->inst_register, 0, -1, 0, ctx->floats_len++, 0, op_type, inst, ctx);
  else lir_addOperandConst(value->kind == Value_UInt ? value->u : value->i, op_type, inst);
}

void op_block(struct LIRInstruction *inst, int64_t value, struct IRType *op_type, struct LIRContext *ctx) {
  int len = getNumCase(value) + 4;
  inst->operands[inst->operands_len++] = (struct LIROperand){
    .kind = LIR_Label,
    .label = malloc(len),
    .size = sizeOf(op_type)
  };
  snprintf(inst->operands[inst->operands_len - 1].label, len, ".Lb%ld", value);
}

void lir_operand(struct LIRInstruction *inst, struct IROperand *operand, struct LIRContext *ctx) {
  if (operand->kind == Operand_Constant)
    op_const(inst, &operand->constant, &operand->op_type, ctx);
  else if (operand->kind == Operand_Register)
    lir_addOperandReg(1, operand->ref, &operand->op_type, is_float(&operand->op_type), inst, ctx);
  else if (operand->kind == Operand_Block)
    op_block(inst, operand->ref, &operand->op_type, ctx);
  else if (operand->kind == Operand_String)
    lir_addOperandLabel(2, operand->string, &operand->op_type, inst);
  else if (operand->kind == Operand_Global)
    lir_addOperandLabel(1, operand->string, &operand->op_type, inst);
}
