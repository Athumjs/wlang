#include "ir.h"
#include <utils/numeric.h>

struct IROperand irAssign(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena);

  if (expr->expr_binary.op == TOKEN_ASSIGN) {
    struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena); 
    
		if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		  struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
		  inst->operands[inst->operands_len++] = refr;
		  refl.kind = Operand_Global;
		  inst->operands[inst->operands_len++] = refl;
		  refl.kind = Operand_Register;
		}
		
		else {
			if (expr->expr_binary.right->kind == Expr_Literal) {
			  struct IRInstruction *inst = newInst(OPCODE_CONST, expr->type, 1, 1, block, arena);
			  inst->operands[inst->operands_len++] = refr;
			}
			
      setCurrentDef(func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &block->instructions[block->instructions_len - 1], block->index, arena);
		}

    return (struct IROperand){};
  }

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
  struct IROperand refr = irExpr(expr, module, func, block, arena);
  expr->kind = Expr_Assign;

  if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
		inst->operands[inst->operands_len++] = refr;
		refl.kind = Operand_Global;
		inst->operands[inst->operands_len++] = refl;
		refl.kind = Operand_Register;
  }

  else
    setCurrentDef(func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &block->instructions[block->instructions_len - 1], block->index, arena);
}

struct IROperand irCompare(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena);
  struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena);
  struct IRInstruction *inst = newInst(OPCODE_ICMP, expr->type, 1, 3, block, arena);

  if (isFloating(expr->expr_binary.left->type)) {
    inst->opcode = OPCODE_FCMP;
    enum IRFCMP opc = FCMP_OEq;

    if (expr->expr_binary.op == TOKEN_NE)
      opc = FCMP_ONe;

    else if (isSignedInteger(expr->type)) {
      if (expr->expr_binary.op == TOKEN_GT)
        opc = FCMP_OGt;
      else if (expr->expr_binary.op == TOKEN_GE)
        opc = FCMP_OGe;
      else if (expr->expr_binary.op == TOKEN_LT)
        opc = FCMP_OLt;
      else if (expr->expr_binary.op == TOKEN_LE)
        opc = FCMP_OLe;
    }

    addOperandFCmp(opc, inst);
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

    addOperandICmp(opc, inst);
  }

  inst->operands[inst->operands_len++] = refl;
  inst->operands[inst->operands_len++] = refr;
  return (struct IROperand){
    .kind = Operand_Register,
    .ref = inst->vl
  };
}

struct IROperand irBinary(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
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

  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena);
  struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena);
  struct IRInstruction *inst = newInst(opc, expr->type, 1, 2, block, arena);
  inst->operands[inst->operands_len++] = refl;
  inst->operands[inst->operands_len++] = refr;

  return (struct IROperand){
    .kind = Operand_Register,
    .ref = inst->vl
  };
}

struct IROperand irId(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct Symbol *symbol = expr->expr_identifier.symbol;
  int64_t vl;

  if (symbol->isGlobal) {
    struct IRGlobal *global = &module->globals[symbol->index];
    struct IRInstruction *inst = newInst(OPCODE_LOAD, symbol->type, 1, 1, block, arena);
    addOperandRef(1, global->vl, inst);
    vl = inst->vl;
  } else {
    struct IRDef *def = getCurrentDef(func->ssa, &symbol->name, block, arena);

    if (def->isParam) {
      vl = def->param->vl;
    } else {
      if (def->inst->opcode == OPCODE_ALLOCA) {
        struct IRInstruction *inst = newInst(OPCODE_LOAD, symbol->type, 1, 1, block, arena);
        addOperandRef(3, def->inst->vl, inst);
        vl = inst->vl;
      } else vl = def->inst->vl;
    } 
  };

  return (struct IROperand){
    .kind = Operand_Register,
    .ref = vl
  };
}

struct IROperand irLiteral(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  return (struct IROperand){
    .kind = Operand_Constant,
    .constant = irNumber(expr)
  };
}

struct IROperand irExpr(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (expr->kind == Expr_Assign) return irAssign(expr, module, func, block, arena);
  else if (expr->kind == Expr_Compare) return irCompare(expr, module, func, block, arena);
  else if (expr->kind == Expr_Binary) return irBinary(expr, module, func, block, arena);
  else if (expr->kind == Expr_Identifier) return irId(expr, module, func, block, arena);
  else if (expr->kind == Expr_Literal) return irLiteral(expr, module, func, block, arena);
}
