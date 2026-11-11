#include "expr.h"
#include <utils/numeric.h>

struct IRExpr irCast(struct Expr *expr, struct MIRContext *ctx) {
  enum IROpcode opc = OPCODE_TRUNC;

  if (isFloating(expr->expr_cast.type)) {
    if (isFloating(expr->expr_cast.value->type)) {
      if (rankType(expr->expr_cast.value->type) > rankType(expr->expr_cast.type))
        opc = OPCODE_FPTRUNC;
      else opc = OPCODE_FPEXT;
    }

    else if (isUnsignedInteger(expr->expr_cast.value->type))
      opc = OPCODE_UI2FP;
    else opc = OPCODE_SI2FP;
  }

  else if (isFloating(expr->expr_cast.value->type)) {
    if (isUnsignedInteger(expr->expr_cast.type))
      opc = OPCODE_FP2UI;
    else opc = OPCODE_FP2SI;
  }

  else if (expr->expr_cast.value->type->kind == Type_Pointer && isInteger(expr->expr_cast.type))
    opc = OPCODE_PTR2INT;
  else if (expr->expr_cast.type->kind == Type_Pointer && isSignedInteger(expr->expr_cast.value->type))
    opc = OPCODE_SI2PTR;
  else if (expr->expr_cast.type->kind == Type_Pointer && isUnsignedInteger(expr->expr_cast.value->type))
    opc = OPCODE_UI2PTR;

  else if (isInteger(expr->expr_cast.type) && isInteger(expr->expr_cast.value->type)) {
    int rank_src = rankType(expr->expr_cast.value->type);
    int rank_dst = rankType(expr->expr_cast.type);

    if (rank_src < rank_dst) {
      if (isUnsignedInteger(expr->expr_cast.value->type))
        opc = OPCODE_ZEXT;
      else opc = OPCODE_SEXT;
    }
  }

  struct IRExpr expr_o = irExpr(expr->expr_cast.value, ctx);
  ctx->block = expr_o.block;
  struct IRInstruction *inst = newInst(opc, 1, 2, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = expr_o.operand;
  addOperandType(expr->expr_cast.type, inst, ctx->arena);

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}

struct IRExpr irUnary(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr expr_o = irExpr(expr->expr_unary.arg, ctx);
  ctx->block = expr_o.block;

  if (expr->expr_unary.op == TOKEN_NOT || expr->expr_unary.op == TOKEN_BIT_NOT) {
    struct IRInstruction *inst = newInst(OPCODE_BIT_XOR, 1, 2, ctx->block, ctx->arena);
    inst->operands[inst->operands_len++] = expr_o.operand;
    inst->operands[inst->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = expr->expr_unary.op == TOKEN_NOT ? 1 : -1
      }
    };

    return (struct IRExpr){
      .inst = inst,
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .op_type = inst->operands[0].op_type,
        .ref = inst->vl
      }
    };
  }

  else if (expr->expr_unary.op == TOKEN_ASTERISK) {
    struct IRInstruction *inst = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
    inst->operands[inst->operands_len++] = expr_o.operand;

    return (struct IRExpr){
      .inst = inst,
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .op_type = inst->operands[0].op_type,
        .ref = inst->vl
      }
    };
  }

  else if (expr->expr_unary.op == TOKEN_MINUS) {
    if (expr_o.operand.constant.kind == Value_Int)
      expr_o.operand.constant.i = -expr_o.operand.constant.i;
    else if (expr_o.operand.constant.kind == Value_Float)
      expr_o.operand.constant.f = -expr_o.operand.constant.f;
    else expr_o.operand.constant.d = -expr_o.operand.constant.d;
  }

  else if (expr->expr_unary.op == TOKEN_INCREMENT || expr->expr_unary.op == TOKEN_DECREMENT) {
    struct IRInstruction *inst = newInst(expr->expr_unary.op == TOKEN_INCREMENT ? OPCODE_ADD : OPCODE_SUB, 1, 2, ctx->block, ctx->arena);
    inst->operands[inst->operands_len++] = expr_o.operand;
    inst->operands[inst->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 1
      }
    };

    struct IROperand res = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    };

    if (!expr->expr_unary.prefix) {
      struct IRDef *def = readVariable(ctx->func->ssa, &expr->expr_unary.arg->expr_identifier.name, resolveTypeIR(expr->expr_unary.arg->expr_identifier.symbol->type, ctx->arena), ctx->block, ctx->arena);
      if (def->isParam)
        res.ref = def->param->vl;
      else res.ref = def->inst->vl;
    }

    if (expr->expr_unary.arg->expr_identifier.symbol->isGlobal) {
      struct IRInstruction *store = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
      store->operands[store->operands_len++] = res;
      expr_o.operand.kind = Operand_Global;
      store->operands[store->operands_len++] = expr_o.operand;
      expr_o.operand.kind = Operand_Register;
      return (struct IRExpr){0};
    } else setCurrentDef(ctx->func->ssa, &expr->expr_unary.arg->expr_identifier.name, 0, inst, ctx->block, ctx->arena);

    return (struct IRExpr){
      .inst = inst,
      .operand = res
    };
  }

  return (struct IRExpr){
    .operand = expr_o.operand
  };
}
