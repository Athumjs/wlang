#include "ir.h"
#include <stdlib.h>

struct IRBasicBlock *irBr(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRExpr expr = irExpr(stmt->stmt_if.condition, ctx);
  ctx->block = expr.block;
  struct IRInstruction *inst = newInst(OPCODE_BR, 0, 3, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = expr.operand;
  int64_t tBlock = newBlock(ctx->func, ctx->arena);
  int64_t fBlock = newBlock(ctx->func, ctx->arena);
  int64_t mBlock = -1;
  addOperandRef(1, tBlock, IRTYPE_NULL, inst);
  addOperandRef(1, fBlock, IRTYPE_NULL, inst);

  struct IRBasicBlock *trueBlock = ctx->func->blocks[tBlock];
  struct IRBasicBlock *falseBlock = ctx->func->blocks[fBlock];
  addPrevBlock(ctx->block, trueBlock, ctx->arena);
  addPrevBlock(ctx->block, falseBlock, ctx->arena);
  sealBlock(ctx->func->ssa, trueBlock, ctx->arena);
  sealBlock(ctx->func->ssa, falseBlock, ctx->arena);
  ctx->block = trueBlock;
  trueBlock = irStmt(stmt->stmt_if.trueBody, ctx);
  ctx->block = expr.block;

  if (stmt->stmt_if.falseBody != NULL) {
    ctx->block = falseBlock;
    falseBlock = irStmt(stmt->stmt_if.falseBody, ctx);
    ctx->block = expr.block;
    if (!trueBlock->term || !falseBlock->term) {
      mBlock = newBlock(ctx->func, ctx->arena);
      if (!trueBlock->term) addPrevBlock(trueBlock, ctx->func->blocks[mBlock], ctx->arena);
      if (!falseBlock->term) addPrevBlock(falseBlock, ctx->func->blocks[mBlock], ctx->arena);
    }
  } else if (!trueBlock->term) addPrevBlock(trueBlock, falseBlock, ctx->arena);

  ctx->block->term = 1;

  if (!trueBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, ctx->func->blocks[trueBlock->index], ctx->arena);
    addOperandRef(1, mBlock == -1 ? fBlock : mBlock, IRTYPE_NULL, jmp);
    trueBlock->term = 1;
  }

  if (stmt->stmt_if.falseBody != NULL && !falseBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, ctx->func->blocks[falseBlock->index], ctx->arena);
    addOperandRef(1, mBlock, IRTYPE_NULL, jmp);
    falseBlock->term = 1;
  }

  if (mBlock != -1)
    sealBlock(ctx->func->ssa, ctx->func->blocks[mBlock], ctx->arena);

  return ctx->func->blocks[mBlock == -1 ? fBlock : mBlock];
}

struct IRBasicBlock *irDoWhile(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, 0, 1, ctx->block, ctx->arena);
  ctx->block->term = 1;

  int64_t bBlock = newBlock(ctx->func, ctx->arena);
  int64_t cBlock = newBlock(ctx->func, ctx->arena);
  int64_t eBlock = newBlock(ctx->func, ctx->arena);
  addOperandRef(1, bBlock, IRTYPE_NULL, inst);

  struct IRBasicBlock *bodyBlock = ctx->func->blocks[bBlock];
  struct IRBasicBlock *condBlock = ctx->func->blocks[cBlock];
  struct IRBasicBlock *exitBlock = ctx->func->blocks[eBlock];
  addPrevBlock(ctx->block, bodyBlock, ctx->arena);
  addPrevBlock(bodyBlock, condBlock, ctx->arena);
  addPrevBlock(condBlock, exitBlock, ctx->arena);
  addPrevBlock(condBlock, bodyBlock, ctx->arena);

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = ctx->func->loop
  };

  ctx->func->loop = &loop;
  struct IRBasicBlock *block = ctx->block;
  ctx->block = bodyBlock;
  bodyBlock = irStmt(stmt->stmt_while.body, ctx);
  ctx->block = bodyBlock;
  ctx->func->loop = loop.prev;
  sealBlock(ctx->func->ssa, condBlock, ctx->arena);

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, bodyBlock, ctx->arena);
    addOperandRef(1, cBlock, IRTYPE_NULL, jmp);
    bodyBlock->term = 1;
  }

  ctx->block = condBlock;
  struct IRExpr irexpr = irExpr(stmt->stmt_while.condition, ctx);
  ctx->block = block;
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, 0, 3, condBlock, ctx->arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(1, bBlock, IRTYPE_NULL, br);
  addOperandRef(1, eBlock, IRTYPE_NULL, br);
  condBlock->term = 1;
  sealBlock(ctx->func->ssa, bodyBlock, ctx->arena);
  sealBlock(ctx->func->ssa, ctx->func->blocks[eBlock], ctx->arena);
  return ctx->func->blocks[eBlock];
}

struct IRBasicBlock *irWhile(struct Stmt *stmt, struct MIRContext *ctx) {
  if (stmt->stmt_while.doWhile)
    return irDoWhile(stmt, ctx);

  struct IRInstruction *inst = newInst(OPCODE_JMP, 0, 1, ctx->block, ctx->arena);
  ctx->block->term = 1;

  int64_t cBlock = newBlock(ctx->func, ctx->arena);
  int64_t bBlock = newBlock(ctx->func, ctx->arena);
  int64_t eBlock = newBlock(ctx->func, ctx->arena);
  addOperandRef(1, cBlock, IRTYPE_NULL, inst);

  struct IRBasicBlock *condBlock = ctx->func->blocks[cBlock];
  struct IRBasicBlock *bodyBlock = ctx->func->blocks[bBlock];
  struct IRBasicBlock *exitBlock = ctx->func->blocks[eBlock];
  addPrevBlock(ctx->block, condBlock, ctx->arena);
  addPrevBlock(condBlock, bodyBlock, ctx->arena);
  addPrevBlock(condBlock, exitBlock, ctx->arena);

  struct IRBasicBlock *block = ctx->block;
  ctx->block = condBlock;
  struct IRExpr irexpr = irExpr(stmt->stmt_while.condition, ctx);
  ctx->block = block;
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, 0, 3, condBlock, ctx->arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(1, bBlock, IRTYPE_NULL, br);
  addOperandRef(1, eBlock, IRTYPE_NULL, br);
  condBlock->term = 1;
  sealBlock(ctx->func->ssa, bodyBlock, ctx->arena);

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = ctx->func->loop
  };

  ctx->func->loop = &loop;
  ctx->block = ctx->func->blocks[bBlock];
  bodyBlock = irStmt(stmt->stmt_while.body, ctx);
  ctx->block = block;
  ctx->func->loop = loop.prev;
  addPrevBlock(bodyBlock, condBlock, ctx->arena);

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, bodyBlock, ctx->arena);
    addOperandRef(1, cBlock, IRTYPE_NULL, jmp);
    bodyBlock->term = 1;
  }

  sealBlock(ctx->func->ssa, condBlock, ctx->arena);
  sealBlock(ctx->func->ssa, exitBlock, ctx->arena);
  return ctx->func->blocks[eBlock];
}

struct IRBasicBlock *irFor(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, 0, 1, ctx->block, ctx->arena);
  ctx->block->term = 1;

  int64_t iBlock = newBlock(ctx->func, ctx->arena);
  int64_t cBlock = newBlock(ctx->func, ctx->arena);
  int64_t bBlock = newBlock(ctx->func, ctx->arena);
  int64_t uBlock = newBlock(ctx->func, ctx->arena);
  int64_t eBlock = newBlock(ctx->func, ctx->arena);
  addOperandRef(1, iBlock, IRTYPE_NULL, inst);

  struct IRBasicBlock *initBlock = ctx->func->blocks[iBlock];
  struct IRBasicBlock *condBlock = ctx->func->blocks[cBlock];
  struct IRBasicBlock *bodyBlock = ctx->func->blocks[bBlock];
  struct IRBasicBlock *updateBlock = ctx->func->blocks[uBlock];
  struct IRBasicBlock *exitBlock = ctx->func->blocks[eBlock];
  addPrevBlock(ctx->block, initBlock, ctx->arena);
  addPrevBlock(initBlock, condBlock, ctx->arena);
  addPrevBlock(condBlock, bodyBlock, ctx->arena);
  addPrevBlock(condBlock, exitBlock, ctx->arena);
  addPrevBlock(bodyBlock, updateBlock, ctx->arena);
  sealBlock(ctx->func->ssa, initBlock, ctx->arena);

  struct IRBasicBlock *block = ctx->block;
  if (stmt->stmt_for.init != NULL) {
    ctx->block = initBlock;
    irDecl(stmt->stmt_for.init, ctx);
    ctx->block = block;
  }

  struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, initBlock, ctx->arena);
  addOperandRef(1, cBlock, IRTYPE_NULL, jmp);
  initBlock->term = 1;

  ctx->block = condBlock;
  struct IRExpr irexpr = irExpr(stmt->stmt_for.condition, ctx);
  ctx->block = block;
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, 0, 3, condBlock, ctx->arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(1, bBlock, IRTYPE_NULL, br);
  addOperandRef(1, eBlock, IRTYPE_NULL, br);
  condBlock->term = 1;
  sealBlock(ctx->func->ssa, bodyBlock, ctx->arena);

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = updateBlock,
    .prev = ctx->func->loop
  };

  ctx->func->loop = &loop;
  ctx->block = bodyBlock;
  bodyBlock = irStmt(stmt->stmt_for.body, ctx);
  ctx->block = block;
  ctx->func->loop = loop.prev;
  addPrevBlock(updateBlock, condBlock, ctx->arena);

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, 0, 1, bodyBlock, ctx->arena);
    addOperandRef(1, uBlock, IRTYPE_NULL, jmp);
    bodyBlock->term = 1;
  }

  if (stmt->stmt_for.update != NULL) {
    ctx->block = updateBlock;
    updateBlock = irExpr(stmt->stmt_for.update, ctx).block;
    ctx->block = block;
  }

  sealBlock(ctx->func->ssa, updateBlock, ctx->arena);
  struct IRInstruction *jmp2 = newInst(OPCODE_JMP, 0, 1, updateBlock, ctx->arena);
  addOperandRef(1, cBlock, IRTYPE_NULL, jmp2);
  updateBlock->term = 1;
  sealBlock(ctx->func->ssa, condBlock, ctx->arena);

  sealBlock(ctx->func->ssa, ctx->func->blocks[eBlock], ctx->arena);
  return ctx->func->blocks[eBlock];
}

struct IRBasicBlock *irRet(struct Stmt *stmt, struct MIRContext *ctx) {
  if (stmt->stmt_return == NULL) {
    newInst(OPCODE_RET, 0, 0, ctx->block, ctx->arena);
    ctx->block->term = 1;
    return ctx->block;
  }

  struct IRExpr expr = irExpr(stmt->stmt_return, ctx);
  ctx->block = expr.block;
  struct IRInstruction *inst = newInst(OPCODE_RET, 0, 1, ctx->block, ctx->arena);
  inst->operands[inst->operands_len++] = expr.operand;
  ctx->block->term = 1;
  return ctx->block;
}

struct IRBasicBlock *irContinue(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, 0, 1, ctx->block, ctx->arena);
  addOperandRef(1, ctx->func->loop->continueBlock->index, IRTYPE_NULL, inst);
  ctx->block->term = 1;
  addPrevBlock(ctx->block, ctx->func->loop->continueBlock, ctx->arena);
  return ctx->func->loop->continueBlock;
}

struct IRBasicBlock *irBreak(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, 0, 1, ctx->block, ctx->arena);
  addOperandRef(1, ctx->func->loop->breakBlock->index, IRTYPE_NULL, inst);
  ctx->block->term = 1;
  addPrevBlock(ctx->block, ctx->func->loop->breakBlock, ctx->arena);
  return ctx->func->loop->breakBlock;
}

struct IRBasicBlock *irBlock(struct Stmt *stmt, struct MIRContext *ctx) {
  struct IRBasicBlock *b = ctx->block;

  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    struct Item *item = &stmt->stmt_block.items[i];
    ctx->block = b;
    if (item->kind == Item_Stmt)
      b = irStmt(item->item_stmt, ctx);
    else irDecl(item->item_decl, ctx);
  }

  return b;
}

struct IRBasicBlock *irStmt(struct Stmt *stmt, struct MIRContext *ctx) {
  if (stmt->kind == Stmt_If) return irBr(stmt, ctx);
  else if (stmt->kind == Stmt_While) return irWhile(stmt, ctx);
  else if (stmt->kind == Stmt_For) return irFor(stmt, ctx);
  else if (stmt->kind == Stmt_Return) return irRet(stmt, ctx);
  else if (stmt->kind == Stmt_Continue) return irContinue(stmt, ctx);
  else if (stmt->kind == Stmt_Break) return irBreak(stmt, ctx);
  else if (stmt->kind == Stmt_Block) return irBlock(stmt, ctx);
  else if (stmt->kind == Stmt_Expr) return irExpr(stmt->stmt_expr, ctx).block;
  return ctx->block;
}
