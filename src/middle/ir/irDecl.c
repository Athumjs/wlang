#include "ir.h"
#include <string.h>

static void instStore(struct Expr *expr, struct IRInstruction *ptr, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) {
  if (ir->instructions_len == ir->instructions_cap) {
    size_t oldCap = ir->instructions_cap;
    ir->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, ir->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, ir->instructions, oldCap * sizeof(struct IRInstruction));
    ir->instructions = temp;
  }

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Store,
    .result = -1,

    .operands[0] = (struct IROperand){
      .kind = Operand_Type,
      .type = expr->type
    },

    .operands[1] = irOperand(expr, module, arena),

    .operands[2] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = (struct Pointer){
        .kind = Pointer_Local,
        .inst = ptr
      }
    },

    .operands_len = 3
  };

  ir->instructions[ir->instructions_len++] = inst;
}

void irLocalVar(struct Decl *decl, struct IRModule *ir, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    if (block->instructions_len == block->instructions_cap) {
      size_t oldCap = block->instructions_cap;
      block->instructions_cap *= 2;
      struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
      memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
      block->instructions = temp;
    }

    struct IRInstruction inst = (struct IRInstruction){
      .opcode = Opcode_Alloca,

      .operands[0] = (struct IROperand){
        .kind = Operand_Type,
        .type = decl->decl_variable.vars[i].symbol->type
      },

      .operands_len = 1
    };

    inst.result = ir->functions[ir->functions_len - 1].regs_len++;
    int index = block->instructions_len++;
    block->instructions[index] = inst;
    decl->decl_variable.vars[i].symbol->ptr.kind = Pointer_Local;
    decl->decl_variable.vars[i].symbol->ptr.inst = &block->instructions[block->instructions_len - 1];

    if (decl->decl_variable.vars[i].expr != NULL)
      instStore(decl->decl_variable.vars[i].expr, &block->instructions[index], ir, block, arena);
  }
}

void irDecl(struct Decl *decl, struct IRModule *ir, struct Arena *arena) {
  struct IRFunction *func = &ir->functions[ir->functions_len - 1];
  struct IRBasicBlock *block = &func->blocks[func->blocks_len - 1];

  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  if (decl->kind == Decl_Variable) irLocalVar(decl, ir, block, arena);
}
