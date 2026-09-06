#include "ir.h"
#include <string.h>
#include <utils/numeric.h>

void irAssign(struct Expr *expr, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) {
  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Store,
    .result = -1,

    .operands[0] = (struct IROperand){
      .kind = Operand_Type,
      .type = expr->type
    },

    .operands[1] = irOperand(expr->expr_binary.right, module, arena),

    .operands[2] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = expr->expr_binary.left->expr_identifier.symbol->ptr
    },

    .operands_len = 3
  };

  ir->instructions[ir->instructions_len++] = inst;
}

void irBinary(struct Expr *expr, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) {
  enum IROpcode opc = Opcode_Add;

  if (expr->expr_binary.op == TOKEN_BIT_AND)
    opc = Opcode_Bit_And;
  else if (expr->expr_binary.op == TOKEN_BIT_OR)
    opc = Opcode_Bit_Or;
  else if (expr->expr_binary.op == TOKEN_BIT_XOR)
    opc = Opcode_Bit_Xor;
  else if (expr->expr_binary.op == TOKEN_SHIFT_LEFT)
    opc = Opcode_Bit_Shl;
  else {
    if (isFloating(expr->type)) {
      if (expr->expr_binary.op == TOKEN_PLUS)
        opc = Opcode_FAdd;
      else if (expr->expr_binary.op == TOKEN_MINUS)
        opc = Opcode_FSub;
      else if (expr->expr_binary.op == TOKEN_ASTERISK)
        opc = Opcode_FMul;
      else if (expr->expr_binary.op == TOKEN_SLASH)
        opc = Opcode_FDiv;
      else if (expr->expr_binary.op == TOKEN_MOD)
        opc = Opcode_FRem;
    } else {
      if (expr->expr_binary.op == TOKEN_MINUS)
        opc = Opcode_Sub;
      else if (expr->expr_binary.op == TOKEN_ASTERISK)
        opc = Opcode_Mul;
      else if (isSignedInteger(expr->type)) {
        if (expr->expr_binary.op == TOKEN_SLASH)
          opc = Opcode_SDiv;
        else if (expr->expr_binary.op == TOKEN_MOD)
          opc = Opcode_SRem;
        else if (expr->expr_binary.op == TOKEN_SHIFT_RIGHT)
          opc = Opcode_Bit_AShr;
      } else {
        if (expr->expr_binary.op == TOKEN_SLASH)
          opc = Opcode_UDiv;
        else if (expr->expr_binary.op == TOKEN_MOD)
          opc = Opcode_URem;
        else if (expr->expr_binary.op == TOKEN_SHIFT_RIGHT)
          opc = Opcode_Bit_LShr;
      }
    }
  }

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = opc,
    .result = module->functions[module->functions_len - 1].regs_len++,
    .operands[0] = irOperand(expr->expr_binary.left, module, arena),
    .operands[1] = irOperand(expr->expr_binary.right, module, arena),
    .operands_len = 2
  };

  ir->instructions[ir->instructions_len++] = inst;
}

void irIdentifier(struct Expr *expr, struct IRModule *ir, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Load,
    .result = ir->functions[ir->functions_len - 1].regs_len++,

    .operands[0] = (struct IROperand){
      .kind = Operand_Type,
      .type = expr->expr_identifier.symbol->type
    },

    .operands_len = 2
  };

  if (expr->expr_identifier.symbol->ptr.kind == Pointer_Global) {
    inst.operands[1] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = (struct Pointer){
        .kind = Pointer_Global,
        .global = expr->expr_identifier.symbol->ptr.global
      }
    };
  } else {
    inst.operands[1] = (struct IROperand){
      .kind = Operand_Register,
      .reg = expr->expr_identifier.symbol->ptr.inst->result,
    };
  } 

  block->instructions[block->instructions_len++] = inst;
}

void irExpr(struct Expr *expr, struct IRModule *ir, struct IRBasicBlock *block, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  if (expr->kind == Expr_Assign) irAssign(expr, ir, block, arena);
  else if (expr->kind == Expr_Binary) irBinary(expr, ir, block, arena);
  else if (expr->kind == Expr_Identifier) irIdentifier(expr, ir, block, arena);
}
