#include "ir.h"
#include <stdlib.h>

struct IRBasicBlock *irBr(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand operand = irExpr(stmt->stmt_if.condition, module, func, block, arena);
  struct IRInstruction *inst = newInst(OPCODE_BR, stmt->stmt_if.condition->type, 0, 3, block, arena);
  inst->operands[inst->operands_len++] = operand;
  int64_t itBlock = newBlock(func, arena);
  int64_t ifBlock = newBlock(func, arena);
  int64_t imBlock = -1;
  addOperandRef(2, itBlock, inst);
  addOperandRef(2, ifBlock, inst);

  struct IRBasicBlock *trueBlock = &func->blocks[itBlock];
  struct IRBasicBlock *falseBlock = &func->blocks[ifBlock];

  addPrevBlock(block, trueBlock, arena);
  addPrevBlock(block, falseBlock, arena);
  irStmt(stmt->stmt_if.trueBody, module, func, trueBlock, arena);

  if (stmt->stmt_if.falseBody != NULL) {
    falseBlock = irStmt(stmt->stmt_if.falseBody, module, func, falseBlock, arena);
    if (!trueBlock->term && !falseBlock->term) {
      imBlock = newBlock(func, arena);
      if (!trueBlock->term) addPrevBlock(trueBlock, &func->blocks[imBlock], arena);
      if (!falseBlock->term) addPrevBlock(falseBlock, &func->blocks[imBlock], arena);
    }
  } else if (!trueBlock->term)
    addPrevBlock(trueBlock, falseBlock, arena);

  block->term = 1;

  if (!trueBlock->term) {
    struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, trueBlock, arena);
    addOperandRef(2, imBlock == -1 ? ifBlock : imBlock, inst);
    trueBlock->term = 1;
  }

  if (stmt->stmt_if.falseBody != NULL && !falseBlock->term) {
    struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, falseBlock, arena);
    addOperandRef(2, imBlock, inst);
    falseBlock->term = 1;
  }

  struct IRBasicBlock *b = &func->blocks[imBlock == -1 ? ifBlock : imBlock];

  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, b, arena);
    uint8_t equal = 1;

    for (int j = 0; j < b->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, b->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue; 
      equal = 0;
      break;
    }

    if (!equal) {
      struct IRInstruction *inst = newInst(OPCODE_PHI, NULL, 1, b->prevs_len, &func->blocks[imBlock == -1 ? ifBlock : imBlock], arena);

      for (int j = 0; j < b->prevs_len; j++) {
        struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, b->prevs[j], arena);
        if (pdef->isParam)
          addOperandPhi(pdef->param->vl, pdef->block, inst);
        else addOperandPhi(pdef->inst->vl, pdef->block, inst);
      }

      setCurrentDef(func->ssa, &func->defs[i]->name, 0, inst, b->index, arena);
    }
  }

  return &func->blocks[imBlock == -1 ? ifBlock : imBlock];
}

struct IRBasicBlock *irDoWhile(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t ibBlock = newBlock(func, arena);
  int64_t icBlock = newBlock(func, arena);
  int64_t ieBlock = newBlock(func, arena);
  addOperandRef(2, ibBlock, inst);

  struct IRBasicBlock *bodyBlock = &func->blocks[ibBlock];
  struct IRBasicBlock *condBlock = &func->blocks[icBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[ieBlock];
  addPrevBlock(block, bodyBlock, arena);
  addPrevBlock(bodyBlock, condBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);

  struct IRInstruction **phis = malloc(func->defs_len * sizeof(struct IRInstruction *));

  for (int i = 0; i < func->defs_len; i++) {
    struct IRInstruction *phi = newInst(OPCODE_PHI, NULL, 1, bodyBlock->prevs_len, bodyBlock, arena);
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, block, arena);

    if (def->isParam)
      addOperandPhi(def->param->vl, def->block, phi);
    else addOperandPhi(def->inst->vl, def->block, phi);

    addOperandPhi(-1, condBlock, phi);
    setCurrentDef(func->ssa, &func->defs[i]->name, 0, phi, ibBlock, arena);
    phis[i] = phi;
  }
 
  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  irStmt(stmt->stmt_while.body, module, func, bodyBlock, arena); 
  func->loop = loop.prev;

  if (!bodyBlock->term) {
    struct IRInstruction *bodyInst = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, icBlock, bodyInst);
    bodyBlock->term = 1;
  }

  struct IROperand operand = irExpr(stmt->stmt_while.condition, module, func, condBlock, arena);
  struct IRInstruction *condInst = newInst(OPCODE_BR, stmt->stmt_while.condition->type, 0, 3, condBlock, arena);
  condInst->operands[condInst->operands_len++] = operand;
  addOperandRef(2, ibBlock, condInst);
  addOperandRef(2, ieBlock, condInst);
  condBlock->term = 1;

  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, bodyBlock, arena);
    uint8_t equal = 1;

    for (int j = 0; j < bodyBlock->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, bodyBlock->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue; 
      equal = 0;
      break;
    }

    if (!equal) {
      struct IRInstruction *phi = phis[i];

      for (int j = 0; j < bodyBlock->prevs_len; j++) {
        struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, bodyBlock->prevs[j], arena);
        if (pdef->isParam)
          phi->operands[phi->operands_len - 1].phi.value = pdef->param->vl;
        else phi->operands[phi->operands_len - 1].phi.value = pdef->inst->vl;
        def->inst->operands[def->inst->operands_len - 1].phi.block = pdef->block;
      }
    }
  }

  free(phis);
  return &func->blocks[ieBlock];
}

struct IRBasicBlock *irWhile(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (stmt->stmt_while.doWhile)
    return irDoWhile(stmt, module, func, block, arena);

  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t icBlock = newBlock(func, arena);
  int64_t ibBlock = newBlock(func, arena);
  int64_t ieBlock = newBlock(func, arena);
  addOperandRef(2, icBlock, inst);

  struct IRBasicBlock *condBlock = &func->blocks[icBlock];
  struct IRBasicBlock *bodyBlock = &func->blocks[ibBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[ieBlock];
  addPrevBlock(block, condBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);
  addPrevBlock(bodyBlock, condBlock, arena);

  for (int i = 0; i < func->defs_len; i++) {
    struct IRInstruction *phi = newInst(OPCODE_PHI, NULL, 1, condBlock->prevs_len, condBlock, arena);
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, block, arena);

    if (def->isParam)
      addOperandPhi(def->param->vl, def->block, phi);
    else addOperandPhi(def->inst->vl, def->block, phi);

    addOperandPhi(-1, bodyBlock, phi);
    setCurrentDef(func->ssa, &func->defs[i]->name, 0, phi, icBlock, arena);
  }

  struct IROperand operand = irExpr(stmt->stmt_while.condition, module, func, condBlock, arena);
  struct IRInstruction *condInst = newInst(OPCODE_BR, stmt->stmt_while.condition->type, 0, 3, condBlock, arena);
  condInst->operands[condInst->operands_len++] = operand;
  addOperandRef(2, ibBlock, condInst);
  addOperandRef(2, ieBlock, condInst);
  condBlock->term = 1;

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = condBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  irStmt(stmt->stmt_while.body, module, func, bodyBlock, arena);
  func->loop = loop.prev;

  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock, arena);
    uint8_t equal = 1;

    for (int j = 0; j < condBlock->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue; 
      equal = 0;
      break;
    }

    if (!equal) {
      for (int j = 0; j < condBlock->prevs_len; j++) {
        struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock->prevs[j], arena);
        if (pdef->isParam)
          def->inst->operands[def->inst->operands_len - 1].phi.value = pdef->param->vl;
        else def->inst->operands[def->inst->operands_len - 1].phi.value = pdef->inst->vl;
        def->inst->operands[def->inst->operands_len - 1].phi.block = pdef->block;
      }
    }
  }

  if (!bodyBlock->term) {
    struct IRInstruction *bodyInst = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, icBlock, bodyInst);
    bodyBlock->term = 1;
  }

  return &func->blocks[ieBlock];
}

struct IRBasicBlock *irFor(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_JMP, NULL, 0, 1, block, arena);
  block->term = 1;

  int64_t iinBlock = newBlock(func, arena);
  int64_t icBlock = newBlock(func, arena);
  int64_t ibBlock = newBlock(func, arena);
  int64_t iiBlock = newBlock(func, arena);
  int64_t ieBlock = newBlock(func, arena);
  addOperandRef(2, iinBlock, inst);

  struct IRBasicBlock *initBlock = &func->blocks[iinBlock];
  struct IRBasicBlock *condBlock = &func->blocks[icBlock];
  struct IRBasicBlock *bodyBlock = &func->blocks[ibBlock];
  struct IRBasicBlock *incBlock = &func->blocks[iiBlock];
  struct IRBasicBlock *exitBlock = &func->blocks[ieBlock];
  addPrevBlock(block, initBlock, arena);
  addPrevBlock(initBlock, condBlock, arena);
  addPrevBlock(incBlock, condBlock, arena);
  addPrevBlock(condBlock, bodyBlock, arena);
  addPrevBlock(condBlock, exitBlock, arena);
  addPrevBlock(bodyBlock, incBlock, arena);

  if (stmt->stmt_for.init != NULL) irDecl(stmt->stmt_for.init, module, func, initBlock, arena);
  struct IRInstruction *initInst = newInst(OPCODE_JMP, NULL, 0, 1, initBlock, arena);
  addOperandRef(2, icBlock, initInst);
  initBlock->term = 1;

  for (int i = 0; i < func->defs_len; i++) {
    struct IRInstruction *phi = newInst(OPCODE_PHI, NULL, 1, condBlock->prevs_len, condBlock, arena);
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, initBlock, arena);

    if (def->isParam)
      addOperandPhi(def->param->vl, def->block, phi);
    else addOperandPhi(def->inst->vl, def->block, phi);

    addOperandPhi(-1, incBlock, phi);
    setCurrentDef(func->ssa, &func->defs[i]->name, 0, phi, icBlock, arena);
  }

  struct IROperand operand = irExpr(stmt->stmt_for.condition, module, func, condBlock, arena);
  struct IRInstruction *condInst = newInst(OPCODE_BR, stmt->stmt_for.condition->type, 0, 3, condBlock, arena);
  condInst->operands[condInst->operands_len++] = operand;
  addOperandRef(2, ibBlock, condInst);
  addOperandRef(2, ieBlock, condInst);
  condBlock->term = 1;

  struct IRLoopContext loop = (struct IRLoopContext){
    .breakBlock = exitBlock,
    .continueBlock = incBlock,
    .prev = func->loop
  };

  func->loop = &loop;
  irStmt(stmt->stmt_for.body, module, func, bodyBlock, arena);
  func->loop = loop.prev;

  if (!bodyBlock->term) {
    struct IRInstruction *bodyInst = newInst(OPCODE_JMP, NULL, 0, 1, bodyBlock, arena);
    addOperandRef(2, iiBlock, bodyInst);
    bodyBlock->term = 1;
  }

  if (stmt->stmt_for.update != NULL) irExpr(stmt->stmt_for.update, module, func, incBlock, arena);
  struct IRInstruction *incInst = newInst(OPCODE_JMP, NULL, 0, 1, incBlock, arena);
  addOperandRef(2, icBlock, incInst);
  incBlock->term = 1;

  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock, arena);
    uint8_t equal = 1;

    for (int j = 0; j < condBlock->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue;
      equal = 0;
      break;
    }

    if (!equal) {
      for (int j = 0; j < condBlock->prevs_len; j++) {
        struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, condBlock->prevs[j], arena);

        if (pdef->isParam)
          def->inst->operands[def->inst->operands_len - 1].phi.value = pdef->param->vl;
        else def->inst->operands[def->inst->operands_len - 1].phi.value = pdef->inst->vl;
        def->inst->operands[def->inst->operands_len - 1].phi.block = pdef->block;
      }
    }
  }

  return &func->blocks[ieBlock];
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

  struct IROperand ref = irExpr(stmt->stmt_return, module, func, block, arena);
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
