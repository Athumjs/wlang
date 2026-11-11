#include "expr.h"

struct IRExpr irMember(struct Expr *expr, struct MIRContext *ctx) {
  if (expr->expr_member.member->expr_identifier.symbol->kind == Symbol_EnumValue)
    return irExpr(expr->expr_member.member->expr_identifier.symbol->symbol_enumvalue.expr, ctx);

  struct IRExpr expr_o = irExpr(expr->expr_member.obj, ctx);
  ctx->block = expr_o.block;

  if (ctx->block->instructions[ctx->block->instructions_len - 1].opcode == OPCODE_LOAD) {
    (*ctx->block->reg)--;
    expr_o.operand.ref = ctx->block->instructions[ctx->block->instructions_len - 1].operands[0].ref;
    ctx->block->instructions_len--;
  }

  struct IRType type = ctx->module->globals[expr->expr_member.obj->expr_identifier.symbol->index].value.type;
  struct IRInstruction *inst = newInst(OPCODE_GEP, 1, 3, ctx->block, ctx->arena);
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
    .op_type = resolveTypeIR(expr->expr_member.member->expr_identifier.symbol->type, ctx->arena),
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = expr->expr_member.member->expr_identifier.symbol->index
    }
  };

  struct IRInstruction *load = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
  addOperandRef(2, inst->vl, inst->operands[0].op_type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = load->operands[0].op_type,
      .ref = load->vl
    }
  };
}

struct IRExpr irIndex(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr expr_b = irExpr(expr->expr_index.base, ctx);
  ctx->block = expr_b.block;

  if (ctx->block->instructions[ctx->block->instructions_len - 1].opcode == OPCODE_LOAD) {
    (*ctx->block->reg)--;
    expr_b.operand.ref = ctx->block->instructions[ctx->block->instructions_len - 1].operands[0].ref;
    ctx->block->instructions_len--;
  }

  struct IRExpr expr_i = irExpr(expr->expr_index.index, ctx);
  ctx->block = expr_i.block;
  struct IRInstruction *inst = newInst(OPCODE_GEP, 1, 3, ctx->block, ctx->arena);
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
  struct IRInstruction *load = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
  addOperandRef(2, inst->vl, inst->operands[0].op_type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = load->operands[0].op_type,
      .ref = load->vl
    }
  };
}

struct IRExpr irCall(struct Expr *expr, struct MIRContext *ctx) {
  struct IROperand operand = (struct IROperand){
    .kind = Operand_Call,
    .op_type = resolveTypeIR(expr->type, ctx->arena),
    .call = (struct IRCall){
      .operand = arena_alloc(ctx->arena, sizeof(struct IROperand)),
      .args = NULL,
      .args_len = 0
    }
  };

  if (expr->expr_call.callee->kind == Expr_Member) {
    *operand.call.operand = (struct IROperand){
      .kind = Operand_String,
      .op_type = resolveTypeIR(expr->expr_call.callee->expr_member.member->expr_identifier.symbol->type, ctx->arena),
      .string = &expr->expr_call.callee->expr_member.member->expr_identifier.symbol->name
    };

    operand.call.args = arena_alloc(ctx->arena, (expr->expr_call.args_len + 1) * sizeof(struct IROperand));
    operand.call.args_len++;
    struct IRDef *def = readVariable(ctx->func->ssa, &expr->expr_call.callee->expr_member.obj->expr_identifier.name, resolveTypeIR(expr->expr_call.callee->expr_member.obj->expr_identifier.symbol->type, ctx->arena), ctx->block, ctx->arena);
    operand.call.args[0] = (struct IROperand){
      .kind = Operand_Register,
      .op_type = def->inst->operands[0].op_type,
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
    struct IRInstruction *inst = newInst(OPCODE_CALL, 0, 1, ctx->block, ctx->arena);
    inst->operands[inst->operands_len++] = operand;
    return (struct IRExpr){0};
  }

  struct IRInstruction *inst = newInst(OPCODE_CALL, 1, 1, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = operand;

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}
