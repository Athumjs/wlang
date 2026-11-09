#include "expr.h"
#include <utils/numeric.h>

struct IRExpr irAssign(struct Expr *expr, struct MIRContext *ctx) {
  if (expr->expr_binary.op == TOKEN_ASSIGN) {
    struct IRExpr expr_r = irExpr(expr->expr_binary.right, ctx); 
    ctx->block = expr_r.block;

    if (expr->expr_binary.left->kind != Expr_Identifier) {
      struct IRExpr expr_l = irExpr(expr->expr_binary.left, ctx);
      ctx->block = expr_l.block;

      if (ctx->block->instructions[ctx->block->instructions_len - 1].opcode == OPCODE_LOAD) {
        (*ctx->block->reg)--;
        expr_r.operand.ref = ctx->block->instructions[ctx->block->instructions_len - 1].operands[0].ref;
        ctx->block->instructions_len--;
      }

		  struct IRInstruction *inst = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
		  inst->operands[inst->operands_len++] = expr_r.operand;
      inst->operands[inst->operands_len++] = expr_l.operand;
    }
    
    else if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		  struct IRInstruction *inst = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
		  inst->operands[inst->operands_len++] = expr_r.operand;
      struct IRGlobal *global = &ctx->module->globals[expr->expr_binary.left->expr_identifier.symbol->index];
      addOperandGlobal(global->name, global->value.op_type, inst);
		} else {
      struct IRDef *def = getCurrentDef(ctx->func->ssa, &expr->expr_binary.left->expr_identifier.name, ctx->block, ctx->arena);

      if (!def->isParam && def->inst->opcode == OPCODE_ALLOCA) {
        struct IRInstruction *inst = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
        inst->operands[inst->operands_len++] = expr_r.operand;
        addOperandRef(2, def->inst->vl, def->inst->operands[0].type, inst);
        return (struct IRExpr){0};
      }

			if (expr->expr_binary.right->kind == Expr_Literal) {
			  struct IRInstruction *inst = newInst(OPCODE_CONST, 1, 1, ctx->block, ctx->arena);
			  inst->operands[inst->operands_len++] = expr_r.operand;
			}
			
      setCurrentDef(ctx->func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &ctx->block->instructions[ctx->block->instructions_len - 1], ctx->block, ctx->arena);
		}

    return (struct IRExpr){0};
  }
  
  struct IRExpr expr_l = irExpr(expr->expr_binary.left, ctx);
  ctx->block = expr_l.block;

  if (expr->expr_binary.op == TOKEN_MINUS_ASSIGN)
    expr->expr_binary.op = TOKEN_MINUS;
  else if (expr->expr_binary.op == TOKEN_ASTERISK_ASSIGN)
    expr->expr_binary.op = TOKEN_ASTERISK;
  else if (expr->expr_binary.op == TOKEN_SLASH_ASSIGN)
    expr->expr_binary.op = TOKEN_SLASH;
  else if (expr->expr_binary.op == TOKEN_MOD_ASSIGN)
    expr->expr_binary.op = TOKEN_MOD;
  else if (expr->expr_binary.op == TOKEN_BIT_AND_ASSIGN)
    expr->expr_binary.op = TOKEN_BIT_AND;
  else if (expr->expr_binary.op == TOKEN_BIT_XOR_ASSIGN)
    expr->expr_binary.op = TOKEN_BIT_XOR;
  else if (expr->expr_binary.op == TOKEN_BIT_OR_ASSIGN)
    expr->expr_binary.op = TOKEN_BIT_OR;

  expr->kind = Expr_Binary;
  struct IRExpr expr_r = irExpr(expr, ctx);
  ctx->block = expr_r.block;
  expr->kind = Expr_Assign;

  if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		struct IRInstruction *inst = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
		inst->operands[inst->operands_len++] = expr_r.operand;
		expr_l.operand.kind = Operand_Global;
		inst->operands[inst->operands_len++] = expr_l.operand;
		expr_l.operand.kind = Operand_Register;
  }

  else
    setCurrentDef(ctx->func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &ctx->block->instructions[ctx->block->instructions_len - 1], ctx->block, ctx->arena);

  return (struct IRExpr){0};
}

struct IRExpr irLogical(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr irexpr = {0};
  struct IRExpr expr_l = irExpr(expr->expr_binary.left, ctx);
  ctx->block = expr_l.block;

  struct IRInstruction *value = newInst(OPCODE_CONST, 1, 1, ctx->block, ctx->arena);
  value->operands[value->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i1,
      .constant = (struct IRValue){
        .kind = Value_Int
      }
  };

  struct IRInstruction *inst = newInst(OPCODE_BR, 0, 3, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = expr_l.operand;
  ctx->block->term = 1;

  int64_t nBlock = newBlock(ctx->func, ctx->arena);
  int64_t mBlock = newBlock(ctx->func, ctx->arena);

  struct IRBasicBlock *newBlock = &ctx->func->blocks[nBlock];
  struct IRBasicBlock *mergeBlock = &ctx->func->blocks[mBlock];
  addPrevBlock(ctx->block, newBlock, ctx->arena);
  addPrevBlock(ctx->block, mergeBlock, ctx->arena);

  if (expr->expr_binary.op == TOKEN_AND) {
    addOperandRef(1, nBlock, IRTYPE_NULL, inst);
    addOperandRef(1, mBlock, IRTYPE_NULL, inst);
  } else {
    addOperandRef(1, mBlock, IRTYPE_NULL, inst);
    addOperandRef(1, nBlock, IRTYPE_NULL, inst);
  }

  struct IRBasicBlock *block = ctx->block;
  ctx->block = newBlock;
  struct IRExpr expr_r = irExpr(expr->expr_binary.right, ctx);
  ctx->block = block;
  newBlock = expr_r.block;
  addPrevBlock(newBlock, mergeBlock, ctx->arena);

  struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, newBlock, ctx->arena);
  addOperandRef(1, mBlock, IRTYPE_NULL, jmp);
  newBlock->term = 1; 

  if (expr->expr_binary.op == TOKEN_AND)
    value->operands[value->operands_len - 1].constant.i = 0;
  else value->operands[value->operands_len - 1].constant.i = 1;

  struct IRInstruction *phi = newInst(OPCODE_PHI, 1, mergeBlock->prevs_len, mergeBlock, ctx->arena);
  addOperandPhi(value->vl, value->operands[0].op_type, block, phi);
  addOperandPhi(expr_r.operand.ref, expr_r.operand.op_type, newBlock, phi);

  irexpr.operand = (struct IROperand){
    .kind = Operand_Register,
    .op_type = phi->operands[0].op_type,
    .ref = phi->vl
  };
  irexpr.block = mergeBlock;
  return irexpr;
}

struct IRExpr irCompare(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr expr_l = irExpr(expr->expr_binary.left, ctx);
  ctx->block = expr_l.block;
  struct IRExpr expr_r = irExpr(expr->expr_binary.right, ctx);
  ctx->block = expr_r.block;
  struct IRInstruction *inst = newInst(OPCODE_ICMP, 1, 3, ctx->block, ctx->arena);

  if (isFloating(expr->expr_binary.left->type)) {
    inst->opcode = OPCODE_FCMP;
    enum IRFCMP opc = FCMP_OEq;

    if (expr->expr_binary.op == TOKEN_NE)
      opc = FCMP_ONe;

    if (expr->expr_binary.op == TOKEN_GT)
      opc = FCMP_OGt;
    else if (expr->expr_binary.op == TOKEN_GE)
      opc = FCMP_OGe;
    else if (expr->expr_binary.op == TOKEN_LT)
      opc = FCMP_OLt;
    else if (expr->expr_binary.op == TOKEN_LE)
      opc = FCMP_OLe;

    addOperandFCmp(opc, resolveTypeIR(expr->type, ctx->arena), inst);
  }

  else {
    enum IRICMP opc = ICMP_Eq;

    if (expr->expr_binary.op == TOKEN_NE)
      opc = ICMP_Ne;

    else if (isSignedInteger(expr->expr_binary.left->type)) {
      if (expr->expr_binary.op == TOKEN_GT)
        opc = ICMP_SGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        opc = ICMP_SGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        opc = ICMP_SLt;
      else if (expr->expr_binary.op == TOKEN_LE)
        opc = ICMP_SLe;
    }

    else {
      if (expr->expr_binary.op == TOKEN_GT)
        opc = ICMP_UGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        opc = ICMP_UGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        opc = ICMP_ULt;
      else if (expr->expr_binary.op == TOKEN_LE)
        opc = ICMP_ULe;
    }

    addOperandICmp(opc, resolveTypeIR(expr->type, ctx->arena), inst);
  }

  inst->operands[inst->operands_len++] = expr_l.operand;
  inst->operands[inst->operands_len++] = expr_r.operand;
  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}

struct IRExpr irBinary(struct Expr *expr, struct MIRContext *ctx) {
  enum IROpcode opc = OPCODE_ADD;

  if (expr->expr_binary.op == TOKEN_BIT_AND)
    opc = OPCODE_BIT_AND;
  else if (expr->expr_binary.op == TOKEN_BIT_XOR)
    opc = OPCODE_BIT_XOR;
  else if (expr->expr_binary.op == TOKEN_BIT_OR)
    opc = OPCODE_BIT_OR;
  else if (expr->expr_binary.op == TOKEN_SHIFT_LEFT)
    opc = OPCODE_BIT_SHL;

  else {
    if (isFloating(expr->type)) {
      if (expr->expr_binary.op == TOKEN_PLUS)
        opc = OPCODE_FADD;
      else if (expr->expr_binary.op == TOKEN_MINUS)
        opc = OPCODE_FSUB;
      else if (expr->expr_binary.op == TOKEN_ASTERISK)
        opc = OPCODE_FMUL;
      else if (expr->expr_binary.op == TOKEN_SLASH)
        opc = OPCODE_FDIV;
      else if (expr->expr_binary.op == TOKEN_MOD)
        opc = OPCODE_FREM;
    }

    else {
      if (expr->expr_binary.op == TOKEN_MINUS)
        opc = OPCODE_SUB;
      else if (expr->expr_binary.op == TOKEN_ASTERISK)
        opc = OPCODE_MUL;

      else if (isSignedInteger(expr->type)) {
        if (expr->expr_binary.op == TOKEN_SLASH)
          opc = OPCODE_SDIV;
        else if (expr->expr_binary.op == TOKEN_MOD)
          opc = OPCODE_SREM;
        else if (expr->expr_binary.op == TOKEN_SHIFT_RIGHT)
          opc = OPCODE_BIT_ASHR;
      }

      else {
        if (expr->expr_binary.op == TOKEN_SLASH)
          opc = OPCODE_UDIV;
        else if (expr->expr_binary.op == TOKEN_MOD)
          opc = OPCODE_UREM;
        else if (expr->expr_binary.op == TOKEN_SHIFT_RIGHT)
          opc = OPCODE_BIT_LSHR;
      }
    }
  }

  struct IRExpr expr_l = irExpr(expr->expr_binary.left, ctx);
  ctx->block = expr_l.block;
  struct IRExpr expr_r = irExpr(expr->expr_binary.right, ctx);
  ctx->block = expr_r.block;
  struct IRInstruction *inst = newInst(opc, 1, 2, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = expr_l.operand;
  inst->operands[inst->operands_len++] = expr_r.operand;

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}
