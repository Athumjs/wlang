#include <middle/irgen.h>
#include <string.h>
#include "ir.h"

void loweringAST(struct Program *program, struct IRModule *ir) {
  for (int i = 0; i < program->length; i++) {
    irGlobal(program->decls[i], ir, program->arena);
  }
}

struct Value irNumber(struct Expr *expr) {
  struct Value value;

  if (expr->expr_literal.kind == LITERAL_INTEGER || expr->expr_literal.kind == LITERAL_BOOLEAN) {
    value.kind = Value_Int;
    value.i = expr->expr_literal.literal.numInt;
  }

  else if (expr->expr_literal.kind == LITERAL_UINTEGER) {
    value.kind = Value_UInt;
    value.u = expr->expr_literal.literal.numUint;
  }

  else if (expr->expr_literal.kind == LITERAL_FLOAT) {
    value.kind = Value_Float;
    value.f = expr->expr_literal.literal.numFloat;
  }
  
  else if (expr->expr_literal.kind == LITERAL_DOUBLE) {
    value.kind = Value_Double;
    value.d = expr->expr_literal.literal.numDouble;
  }

  return value;
}

struct IROperand irOperand(struct Expr *expr, struct IRModule *ir, struct Arena *arena) {
  if (expr->kind == Expr_Literal) {
    return (struct IROperand){
      .kind = Operand_Constant,
      .constant = irNumber(expr)
    };
  }

  struct IRFunction *func = &ir->functions[ir->functions_len - 1];
  irExpr(expr, ir, &func->blocks[func->blocks_len - 1], arena);
  return (struct IROperand){
    .kind = Operand_Register,
    .reg = func->regs_len - 1
  };
}

static void instStore(struct Type *type, struct IRGlobal *ptr, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) {
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
      .type = type
    },

    .operands[1] = (struct IROperand){
      .kind = Operand_Register,
      .reg = module->init.regs_len - 1
    },

    .operands[2] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = (struct Pointer){
        .kind = Pointer_Global,
        .global = ptr
      }
    },

    .operands_len = 3
  };

  ir->instructions[ir->instructions_len++] = inst;
}

void instRet(struct IRBasicBlock *ir, struct Arena *arena) {
  if (ir->instructions_len == ir->instructions_cap) {
    size_t oldCap = ir->instructions_cap;
    ir->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, ir->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, ir->instructions, oldCap * sizeof(struct IRInstruction));
    ir->instructions = temp;
  }

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Ret,
    .result = -1,
    .operands_len = 0
  };

  ir->instructions[ir->instructions_len++] = inst;
}

struct Value irValue(int id, struct Expr *expr, struct IRModule *ir, struct Arena *arena) {
  if (expr->kind == Expr_Literal)
    return irNumber(expr);

  if (ir->init.blocks_len == 0) {
    struct IRBasicBlock block = {
      .instructions_cap = 4,
      .instructions = arena_alloc(arena, 4 * sizeof(struct IRInstruction)),
      .instructions_len = 0
    };

    ir->init.blocks[0] = block;
    ir->init.blocks_len++;
  }

  irExpr(expr, ir, &ir->init.blocks[0], arena);
  struct IRGlobal *ptr = &ir->globals[id];
  instStore(expr->type, ptr, ir, &ir->init.blocks[0], arena);
  instRet(&ir->init.blocks[0], arena);

  return (struct Value){
    .kind = Value_Int,
    .i = 0
  };
}
