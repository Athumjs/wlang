#include "ir.h"
#include <stdio.h>
#include <string.h>

int newBlock(struct IRFunction *ir, struct Arena *arena) {
  if (ir->blocks_len == ir->blocks_cap) {
    size_t oldCap = ir->blocks_cap;
    ir->blocks_cap *= 2;
    struct IRBasicBlock *temp = arena_alloc(arena, ir->blocks_cap * sizeof(struct IRBasicBlock));
    memcpy(temp, ir->blocks, oldCap * sizeof(struct IRBasicBlock));
    ir->blocks = temp;
  }

  struct IRBasicBlock block = (struct IRBasicBlock){
    .instructions_cap = 8,
    .instructions = arena_alloc(arena, 8 * sizeof(struct IRInstruction)),
    .instructions_len = 0
  };

  ir->blocks[ir->blocks_len++] = block;
  return ir->blocks_len - 1;
}

void instJmp(int jmp, struct IRBasicBlock *ir, struct Arena *arena) {
  if (ir->instructions_len == ir->instructions_cap) {
    size_t oldCap = ir->instructions_cap;
    ir->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, ir->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, ir->instructions, oldCap * sizeof(struct IRInstruction));
    ir->instructions = temp;
  }

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Jmp,
    .result = -1,

    .operands[0] = (struct IROperand){
      .kind = Operand_Block,
      .block = jmp
    },

    .operands_len = 1
  };

  ir->instructions[ir->instructions_len++] = inst;
  ir->term = 1;
}

void irBr(struct Stmt *stmt, struct IRModule *module, struct IRFunction *ir, struct Arena *arena) {
  int index = ir->blocks_len - 1;

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Br,
    .result = -1,
    .operands[0] = irOperand(stmt->stmt_if.condition, module, arena),
    .operands_len = 3,
  };

  inst.operands[1] = (struct IROperand){
    .kind = Operand_Block,
    .block = newBlock(ir, arena)
  };
  irStmt(stmt->stmt_if.trueBody, module, arena);

  inst.operands[2] = (struct IROperand){
      .kind = Operand_Block,
      .block = newBlock(ir, arena)
  };

  if (stmt->stmt_if.falseBody != NULL) {
    irStmt(stmt->stmt_if.falseBody, module, arena);
    if (stmt->stmt_if.falseBody->kind != Stmt_If && !ir->blocks[inst.operands[1].block].term && !ir->blocks[inst.operands[2].block].term)
      newBlock(ir, arena);
  }

  ir->blocks[index].instructions[ir->blocks[index].instructions_len++] = inst;
  ir->blocks[index].term = 1;
  if (!ir->blocks[inst.operands[1].block].term) instJmp(ir->blocks_len - 1, &ir->blocks[inst.operands[1].block], arena);
  if (stmt->stmt_if.falseBody != NULL && !ir->blocks[inst.operands[2].block].term)
    instJmp(ir->blocks_len - 1, &ir->blocks[inst.operands[2].block], arena);
}

void irRet(struct Stmt *stmt, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) { 
  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Ret,
    .result = -1,
    .operands[0] = irOperand(stmt->stmt_return, module, arena),
    .operands_len = 1,
  };

  ir->instructions[ir->instructions_len++] = inst;
  ir->term = 1;
}

void irBlock(struct Stmt *stmt, struct IRModule *module, struct IRFunction *ir, struct Arena *arena) {
  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    if (stmt->stmt_block.items[i].kind == Item_Decl)
      irDecl(stmt->stmt_block.items[i].item_decl, module, arena);
    else irStmt(stmt->stmt_block.items[i].item_stmt, module, arena);
  }
}

void irStmt(struct Stmt *stmt, struct IRModule *ir, struct Arena *arena) { 
  struct IRFunction *func = &ir->functions[ir->functions_len - 1];
  struct IRBasicBlock *block = &func->blocks[func->blocks_len - 1];

  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  if (stmt->kind == Stmt_If) irBr(stmt, ir, func, arena);
  else if (stmt->kind == Stmt_Return) irRet(stmt, ir, block, arena);
  else if (stmt->kind == Stmt_Block) irBlock(stmt, ir, func, arena);
  else if (stmt->kind == Stmt_Expr) irExpr(stmt->stmt_expr, ir, block, arena);
}
