#include "ir.h"
#include <stdlib.h>

struct IRBasicBlock *irBr(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRExpr expr = irExpr(stmt->stmt_if.condition, module, func, block, arena);
  block = expr.block;
  struct IRInstruction *inst = newInst(OPCODE_BR, stmt->stmt_if.condition->type, 0, 3, block, arena);
  inst->operands[inst->operands_len++] = expr.operand;
  int64_t tBlock = newBlock(func, arena);
  int64_t fBlock = newBlock(func, arena);
  int64_t mBlock = -1;
  addOperandRef(2, tBlock, inst);
  addOperandRef(2, fBlock, inst);

  struct IRBasicBlock *trueBlock = &func->blocks[tBlock];
  struct IRBasicBlock *falseBlock = &func->blocks[fBlock];
  addPrevBlock(block, trueBlock, arena);
  addPrevBlock(block, falseBlock, arena);
  trueBlock = irStmt(stmt->stmt_if.trueBody, module, func, trueBlock, arena);

  if (stmt->stmt_if.falseBody != NULL) {
    falseBlock = irStmt(stmt->stmt_if.falseBody, module, func, falseBlock, arena);
    if (!trueBlock->term || !falseBlock->term) {
      mBlock = newBlock(func, arena);
      if (!trueBlock->term) addPrevBlock(trueBlock, &func->blocks[mBlock], arena);
      if (!falseBlock->term) addPrevBlock(falseBlock, &func->blocks[mBlock], arena);
    }
  } else if (!trueBlock->term) addPrevBlock(trueBlock, falseBlock, arena);

  block->term = 1;

  if (!trueBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, &func->blocks[trueBlock->index], arena);
    addOperandRef(2, mBlock == -1 ? fBlock : mBlock, jmp);
    trueBlock->term = 1;
  }

  if (stmt->stmt_if.falseBody != NULL && !falseBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, &func->blocks[falseBlock->index], arena);
    addOperandRef(2, mBlock, jmp);
    falseBlock->term = 1;
  }

  resolvePhi(func, &func->blocks[mBlock == -1 ? fBlock : mBlock], arena);
  return &func->blocks[mBlock == -1 ? fBlock : mBlock];
}

struct IRBasicBlock *irDoWhile(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t bBlock = newBlock(func, arena);
  int64_t cBlock = newBlock(func, arena);
  int64_t eBlock = newBlock(func, arena);
  addOperandRef(2, bBlock, inst);

  struct IRBasicBlock *bodyBlock = &func->blocks[bBlock];
  struct IRBasicBlock *condBlock = &func->blocks[cBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[eBlock];
  addPrevBlock(block, bodyBlock, arena);
  addPrevBlock(bodyBlock, condBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);

  struct IRInstruction **phis = malloc(func->defs_len * sizeof(struct IRInstruction *));
  addPhiImcompletes(func, block, bodyBlock, condBlock, arena, &phis);

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  bodyBlock = irStmt(stmt->stmt_while.body, module, func, bodyBlock, arena);
  func->loop = loop.prev;

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, cBlock, jmp);
    bodyBlock->term = 1;
  }

  struct IRExpr irexpr = irExpr(stmt->stmt_while.condition, module, func, condBlock, arena);
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, stmt->stmt_while.condition->type, 0, 3, condBlock, arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(2, bBlock, br);
  addOperandRef(2, eBlock, br);
  condBlock->term = 1;
  resolvePhiIncompletes(func, bodyBlock, arena, &phis);
  free(phis);
  return &func->blocks[eBlock];
}

struct IRBasicBlock *irWhile(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (stmt->stmt_while.doWhile)
    return irDoWhile(stmt, module, func, block, arena);

  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t cBlock = newBlock(func, arena);
  int64_t bBlock = newBlock(func, arena);
  int64_t eBlock = newBlock(func, arena);
  addOperandRef(2, cBlock, inst);

  struct IRBasicBlock *condBlock = &func->blocks[cBlock];
  struct IRBasicBlock *bodyBlock = &func->blocks[bBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[eBlock];
  addPrevBlock(block, condBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);

  struct IRInstruction **phis = malloc(func->defs_len * sizeof(struct IRInstruction *));
  addPhiImcompletes(func, block, condBlock, bodyBlock, arena, &phis);

  struct IRExpr irexpr = irExpr(stmt->stmt_while.condition, module, func, condBlock, arena);
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, stmt->stmt_while.condition->type, 0, 3, condBlock, arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(2, bBlock, br);
  addOperandRef(2, eBlock, br);
  condBlock->term = 1;

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  bodyBlock = irStmt(stmt->stmt_while.body, module, func, &func->blocks[bBlock], arena);
  func->loop = loop.prev;
  addPrevBlock(bodyBlock, condBlock, arena);

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, cBlock, jmp);
    bodyBlock->term = 1;
  }

  resolvePhiIncompletes(func, condBlock, arena, &phis);
  free(phis);
  return &func->blocks[eBlock];
}

struct IRBasicBlock *irFor(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t iBlock = newBlock(func, arena);
  int64_t cBlock = newBlock(func, arena);
  int64_t bBlock = newBlock(func, arena);
  int64_t uBlock = newBlock(func, arena);
  int64_t eBlock = newBlock(func, arena);
  addOperandRef(2, iBlock, inst);

  struct IRBasicBlock *initBlock = &func->blocks[iBlock];
  struct IRBasicBlock *condBlock = &func->blocks[cBlock];
  struct IRBasicBlock *bodyBlock = &func->blocks[bBlock];
  struct IRBasicBlock *updateBlock = &func->blocks[uBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[eBlock];
  addPrevBlock(block, initBlock, arena);
  addPrevBlock(initBlock, condBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);
  addPrevBlock(bodyBlock, updateBlock, arena);

  if (stmt->stmt_for.init != NULL) irDecl(stmt->stmt_for.init, module, func, initBlock, arena);
  struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, initBlock, arena);
  addOperandRef(2, cBlock, jmp);
  initBlock->term = 1;

  struct IRInstruction **phis = malloc(func->defs_len * sizeof(struct IRInstruction *));
  addPhiImcompletes(func, initBlock, condBlock, updateBlock, arena, &phis);

  struct IRExpr irexpr = irExpr(stmt->stmt_for.condition, module, func, condBlock, arena);
  condBlock = irexpr.block;
  struct IRInstruction *br = newInst(OPCODE_BR, stmt->stmt_for.condition->type, 0, 3, condBlock, arena);
  br->operands[br->operands_len++] = irexpr.operand;
  addOperandRef(2, bBlock, br);
  addOperandRef(2, eBlock, br);
  condBlock->term = 1;

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = updateBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  bodyBlock = irStmt(stmt->stmt_for.body, module, func, bodyBlock, arena);
  func->loop = loop.prev;
  addPrevBlock(updateBlock, condBlock, arena);

  if (!bodyBlock->term) {
    struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, uBlock, jmp);
    bodyBlock->term = 1;
  }

  if (stmt->stmt_for.update != NULL) updateBlock = irExpr(stmt->stmt_for.update, module, func, updateBlock, arena).block;
  struct IRInstruction *jmp2 = newInst(OPCODE_JMP, NULL, 0, 1, updateBlock, arena);
  addOperandRef(2, cBlock, jmp2);
  updateBlock->term = 1;

  resolvePhiIncompletes(func, condBlock, arena, &phis);
  free(phis);
  return &func->blocks[eBlock];
}

void irContinue(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  addOperandRef(2, func->loop->continueBlock->index, inst);
  block->term = 1;
}

void irBreak(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  addOperandRef(2, func->loop->breakBlock->index, inst);
  block->term = 1;
}

void irRet(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (stmt->stmt_return == NULL) {
    newInst(OPCODE_RET, NULL, 0, 0, block, arena);
    block->term = 1;
    return;
  }

  struct IROperand ref = irExpr(stmt->stmt_return, module, func, block, arena).operand;
  struct IRInstruction *inst = newInst(OPCODE_RET, stmt->stmt_return->type, 0, 1, block, arena);
  inst->operands[inst->operands_len++] = ref;
  block->term = 1;
}

struct IRBasicBlock *irBlock(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRBasicBlock *b = block;

  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    struct Item *item = &stmt->stmt_block.items[i];
    if (item->kind == Item_Stmt)
      b = irStmt(item->item_stmt, module, func, b, arena);
    else irDecl(item->item_decl, module, func, b, arena);
  }

  return b;
}

struct IRBasicBlock *irStmt(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (stmt->kind == Stmt_If) return irBr(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_While) return irWhile(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_For) return irFor(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_Return) irRet(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_Continue) irContinue(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_Break) irBreak(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_Block) return irBlock(stmt, module, func, block, arena);
  else if (stmt->kind == Stmt_Expr) irExpr(stmt->stmt_expr, module, func, block, arena);
  return block;
}
