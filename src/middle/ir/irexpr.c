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

void irCompare(struct Expr *expr, struct IRModule *module, struct IRBasicBlock *ir, struct Arena *arena) {
  struct IRInstruction inst = (struct IRInstruction){
    .operands[1] = irOperand(expr->expr_binary.left, module, arena),
    .operands[2] = irOperand(expr->expr_binary.right, module, arena),
    .operands_len = 3
  };

  if (isFloating(expr->expr_binary.left->type)) {
    inst.opcode = Opcode_Fcmp;
    enum FCondition op = FCond_OEq;

    if (expr->expr_binary.op == TOKEN_NE)
      op = FCond_ONe;
    else if (isSignedInteger(expr->type)) {
      if (expr->expr_binary.op == TOKEN_GT)
        op = FCond_OGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        op = FCond_OGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        op = FCond_OLt;
      else if (expr->expr_binary.op == TOKEN_LE)
        op = FCond_OLe;
    }

    inst.operands[0] = (struct IROperand){
      .kind = Operand_FCond,
      .fcond = op
    };
  } else {
    inst.opcode = Opcode_Icmp;
    enum ICondition op = ICond_Eq;

    if (expr->expr_binary.op == TOKEN_NE)
      op = ICond_Ne;
    else if (isSignedInteger(expr->expr_binary.left->type)) {
      if (expr->expr_binary.op == TOKEN_GT)
        op = ICond_SGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        op = ICond_SGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        op = ICond_SLt;
      else if (expr->expr_binary.op == TOKEN_LE)
        op = ICond_SLe;
    } else {
      if (expr->expr_binary.op == TOKEN_GT)
        op = ICond_UGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        op = ICond_UGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        op = ICond_ULt;
      else if (expr->expr_binary.op == TOKEN_LE)
        op = ICond_ULe;
    }

    inst.operands[0] = (struct IROperand){
      .kind = Operand_ICond,
      .icond = op
    };
  } 

  inst.result = module->functions[module->functions_len - 1].regs_len++;
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
    .operands[0] = irOperand(expr->expr_binary.left, module, arena),
    .operands[1] = irOperand(expr->expr_binary.right, module, arena),
    .operands_len = 2
  };

  inst.result = module->functions[module->functions_len - 1].regs_len++;
  ir->instructions[ir->instructions_len++] = inst;
}

void irIdentifier(struct Expr *expr, struct IRModule *ir, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction inst = (struct IRInstruction){
    .opcode = Opcode_Load,

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
  } else if (expr->expr_identifier.symbol->ptr.kind == Pointer_Local) {
    inst.operands[1] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = (struct Pointer){
        .kind = Pointer_Local,
        .inst = expr->expr_identifier.symbol->ptr.inst
      }
    };
  } else {
    inst.operands[1] = (struct IROperand){
      .kind = Operand_Pointer,
      .pointer = (struct Pointer){
        .kind = Pointer_Param,
        .param = expr->expr_identifier.symbol->ptr.param
      }
    };
  }

  inst.result = ir->functions[ir->functions_len - 1].regs_len++;
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
  else if (expr->kind == Expr_Compare) irCompare(expr, ir, block, arena);
  else if (expr->kind == Expr_Binary) irBinary(expr, ir, block, arena);
  else if (expr->kind == Expr_Identifier) irIdentifier(expr, ir, block, arena);
}
