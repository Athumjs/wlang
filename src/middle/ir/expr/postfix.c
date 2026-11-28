#include "expr.h"

struct IRExpr irMember(struct Expr *expr, struct MIRContext *ctx) {
  if (expr->expr_member.member->expr_identifier.symbol->kind == Symbol_EnumValue)
    return irExpr(expr->expr_member.member->expr_identifier.symbol->symbol_enumvalue.expr, ctx);

  struct IRExpr expr_o = irExpr(expr->expr_member.obj, ctx);
  ctx->block = expr_o.block;

  struct IRInstruction *inst = newInst(OPCODE_GEP, NULL, 1, 3, ctx->block, ctx);
  struct IRInstruction *ptr_inst = getInst(ctx->func, expr_o.operand.ref);
  inst->elem_type = ptr_inst->elem_type.kind == IRTYPEK_NULL ? ptr_inst->type : ptr_inst->elem_type;
  inst->operands[inst->operands_len++] = expr_o.operand;

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .op_type.kind = IRTYPEK_OTHER,
    .op_type.other = IRTYPE_i32,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .op_type.kind = IRTYPEK_OTHER,
    .op_type.other = IRTYPE_i32,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = expr->expr_member.member->expr_identifier.symbol->index
    }
  };

  if (expr->type->kind == Type_Array || (expr->type->kind == Type_Named && expr->type->type_named.type == NULL)) {
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .ref = inst->vl
      }
    };
  }

  struct IRInstruction *load = newInst(OPCODE_LOAD, expr->type, 1, 1, ctx->block, ctx);
  addOperandRef(2, inst->vl, inst->type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = load->vl
    }
  };
}

struct IRExpr irIndex(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr expr_b = irExpr(expr->expr_index.base, ctx);
  ctx->block = expr_b.block;

  struct IRExpr expr_i = irExpr(expr->expr_index.index, ctx);
  ctx->block = expr_i.block;
  uint8_t is_ptr = expr->expr_index.base->type->kind == Type_Pointer;
  struct IRInstruction *inst = newInst(OPCODE_GEP, NULL, 1, is_ptr ? 2 : 3, ctx->block, ctx);

  if (is_ptr) {
    inst->elem_type = resolveTypeIR(ctx->module, expr->type, ctx->arena);
    inst->operands[inst->operands_len++] = expr_b.operand;
    inst->operands[inst->operands_len++] = expr_i.operand;
  } else {
    inst->elem_type = resolveTypeIR(ctx->module, expr->expr_index.base->type, ctx->arena);
    inst->operands[inst->operands_len++] = expr_b.operand;

    inst->operands[inst->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    inst->operands[inst->operands_len++] = expr_i.operand;
  }

  if (expr->type->kind == Type_Array || (expr->type->kind == Type_Named && expr->type->type_named.type == NULL)) {
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .ref = inst->vl
      }
    };
  }
 
  struct IRInstruction *load = newInst(OPCODE_LOAD, expr->type, 1, 1, ctx->block, ctx);
  addOperandRef(2, inst->vl, inst->type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = load->vl
    }
  };
}

struct IRExpr irCall(struct Expr *expr, struct MIRContext *ctx) {
  struct IROperand operand = (struct IROperand){
    .kind = Operand_Call,
    .op_type = resolveTypeIR(ctx->module, expr->type, ctx->arena),
    .call = (struct IRCall){
      .operand = arena_alloc(ctx->arena, sizeof(struct IROperand)),
      .args = NULL,
      .args_len = 0,
      .is_extern = expr->expr_call.is_extern
    }
  };

  if (expr->expr_call.callee->kind == Expr_Member) {
    *operand.call.operand = (struct IROperand){
      .kind = Operand_String,
      .op_type = resolveTypeIR(ctx->module, expr->expr_call.callee->expr_member.member->expr_identifier.symbol->type, ctx->arena),
      .string = &expr->expr_call.callee->expr_member.member->expr_identifier.symbol->name
    };

    operand.call.args = arena_alloc(ctx->arena, (expr->expr_call.args_len + 1) * sizeof(struct IROperand));
    operand.call.args_len++;
    struct IRDef *def = readVariable(ctx->func->ssa, &expr->expr_call.callee->expr_member.obj->expr_identifier.name, resolveTypeIR(ctx->module, expr->expr_call.callee->expr_member.obj->expr_identifier.symbol->type, ctx->arena), ctx->block, ctx->arena);
    operand.call.args[0] = (struct IROperand){
      .kind = Operand_Register,
      .op_type = def->inst->type,
      .ref = def->inst->vl
    };
  } else {
    struct IRExpr expr_c = irExpr(expr->expr_call.callee, ctx);
    ctx->block = expr_c.block;
    *operand.call.operand = expr_c.operand;
    operand.call.args = arena_alloc(ctx->arena, expr->expr_call.args_len * sizeof(struct IROperand));
  }

  for (int i = 0; i < expr->expr_call.args_len; i++) {
    struct IRExpr expr_a = irExpr(expr->expr_call.args[i], ctx);
    ctx->block = expr_a.block;
    operand.call.args[operand.call.args_len++] = expr_a.operand;
  }

  if (cmpTP(expr->type, Primitive_Void)) {
    struct IRInstruction *inst = newInst(OPCODE_CALL, expr->type, 0, 1, ctx->block, ctx);
    inst->operands[inst->operands_len++] = operand;
    return (struct IRExpr){0};
  }

  struct IRInstruction *inst = newInst(OPCODE_CALL, expr->type, 1, 1, ctx->block, ctx);
  inst->operands[inst->operands_len++] = operand;

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}
