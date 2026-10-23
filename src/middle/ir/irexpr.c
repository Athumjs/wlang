#include "ir.h"
#include <stdio.h>
#include <string.h>
#include <utils/numeric.h>

struct IRExpr irAssign(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (expr->expr_binary.op == TOKEN_ASSIGN) {
    struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena).operand; 

    if (expr->expr_binary.left->kind != Expr_Identifier) {
      struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena).operand;

      if (block->instructions[block->instructions_len - 1].opcode == OPCODE_LOAD) {
        (*block->reg)--;
        refl.ref = block->instructions[block->instructions_len - 1].operands[0].ref;
        block->instructions_len--;
      }

		  struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
		  inst->operands[inst->operands_len++] = refr;
      inst->operands[inst->operands_len++] = refl;
    }
    
    else if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		  struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
		  inst->operands[inst->operands_len++] = refr;
      struct IRGlobal *global = &module->globals[expr->expr_binary.left->expr_identifier.symbol->index];
      addOperandRef(1, global->vl, inst);
		} else {
      struct IRDef *def = getCurrentDef(func->ssa, &expr->expr_binary.left->expr_identifier.name, block, arena);

      if (!def->isParam && def->inst->opcode == OPCODE_ALLOCA) {
        struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
        inst->operands[inst->operands_len++] = refr;
        addOperandRef(3, def->inst->vl, inst);
        return (struct IRExpr){0};
      }

			if (expr->expr_binary.right->kind == Expr_Literal) {
			  struct IRInstruction *inst = newInst(OPCODE_CONST, expr->type, 1, 1, block, arena);
			  inst->operands[inst->operands_len++] = refr;
			}
			
      setCurrentDef(func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &block->instructions[block->instructions_len - 1], block, arena);
		}

    return (struct IRExpr){0};
  }
  
  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena).operand;

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
  struct IROperand refr = irExpr(expr, module, func, block, arena).operand;
  expr->kind = Expr_Assign;

  if (expr->expr_binary.left->expr_identifier.symbol->isGlobal) {
		struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
		inst->operands[inst->operands_len++] = refr;
		refl.kind = Operand_Global;
		inst->operands[inst->operands_len++] = refl;
		refl.kind = Operand_Register;
  }

  else
    setCurrentDef(func->ssa, &expr->expr_binary.left->expr_identifier.name, 0, &block->instructions[block->instructions_len - 1], block, arena);

  return (struct IRExpr){0};
}

struct IRExpr irLogical(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRExpr irexpr = {0};
  struct IRExpr refl = irExpr(expr->expr_binary.left, module, func, block, arena);
  block = refl.block;

  struct IRInstruction *value = newInst(OPCODE_CONST, expr->type, 1, 1, block, arena);
  value->operands[value->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int
      }
  };

  struct IRInstruction *inst = newInst(OPCODE_BR, expr->type, 0, 3, block, arena);
  inst->operands[inst->operands_len++] = refl.operand;
  block->term = 1;

  int64_t nBlock = newBlock(func, arena);
  int64_t mBlock = newBlock(func, arena);

  struct IRBasicBlock *newBlock = &func->blocks[nBlock];
  struct IRBasicBlock *mergeBlock = &func->blocks[mBlock];
  addPrevBlock(block, newBlock, arena);
  addPrevBlock(block, mergeBlock, arena);

  if (expr->expr_binary.op == TOKEN_AND) {
    addOperandRef(2, nBlock, inst);
    addOperandRef(2, mBlock, inst);
  } else {
    addOperandRef(2, mBlock, inst);
    addOperandRef(2, nBlock, inst);
  }

  struct IRExpr refr = irExpr(expr->expr_binary.right, module, func, newBlock, arena);
  newBlock = refr.block;
  addPrevBlock(newBlock, mergeBlock, arena);

  struct IRInstruction *jmp = newInst(OPCODE_JMP, NULL, 0, 1, newBlock, arena);
  addOperandRef(2, mBlock, jmp);
  newBlock->term = 1; 

  if (expr->expr_binary.op == TOKEN_AND)
    value->operands[value->operands_len - 1].constant.i = 0;
  else value->operands[value->operands_len - 1].constant.i = 1;

  struct IRInstruction *phi = newInst(OPCODE_PHI, NULL, 1, mergeBlock->prevs_len, mergeBlock, arena);
  addOperandPhi(value->vl, block, phi);
  addOperandPhi(refr.operand.ref, newBlock, phi);

  irexpr.operand = (struct IROperand){
    .kind = Operand_Register,
    .ref = phi->vl
  };
  irexpr.block = mergeBlock;
  return irexpr;
}

struct IRExpr irCompare(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena).operand;
  struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena).operand;
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
  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irBinary(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
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

  struct IROperand refl = irExpr(expr->expr_binary.left, module, func, block, arena).operand;
  struct IROperand refr = irExpr(expr->expr_binary.right, module, func, block, arena).operand;
  struct IRInstruction *inst = newInst(opc, expr->type, 1, 2, block, arena);
  inst->operands[inst->operands_len++] = refl;
  inst->operands[inst->operands_len++] = refr;

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irCast(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  enum IROpcode opc = OPCODE_TRUNC;

  if (isFloating(expr->expr_cast.type)) {
    if (isFloating(expr->expr_cast.value->type)) {
      if (rankType(expr->expr_cast.value->type) > rankType(expr->expr_cast.type))
        opc = OPCODE_FPTRUNC;
      else opc = OPCODE_FPEXT;
    }

    else if (isUnsignedInteger(expr->expr_cast.value->type))
      opc = OPCODE_UI2FP;
    else opc = OPCODE_SI2FP;
  }

  else if (isFloating(expr->expr_cast.value->type)) {
    if (isUnsignedInteger(expr->expr_cast.type))
      opc = OPCODE_FP2UI;
    else opc = OPCODE_FP2SI;
  }

  else if (expr->expr_cast.value->type->kind == Type_Pointer && isInteger(expr->expr_cast.type))
    opc = OPCODE_PTR2INT;
  else if (expr->expr_cast.type->kind == Type_Pointer && isInteger(expr->expr_cast.value->type))
    opc = OPCODE_INT2PTR;
  else if (cmpTP(expr->expr_cast.value->type, Primitive_Char) && isInteger(expr->expr_cast.type))
    opc = OPCODE_FPEXT;
  else if (cmpTP(expr->expr_cast.type, Primitive_Char) && isInteger(expr->expr_cast.value->type))
    opc = OPCODE_TRUNC;

  struct IROperand operand = irExpr(expr->expr_cast.value, module, func, block, arena).operand;
  struct IRInstruction *inst = newInst(opc, expr->expr_cast.value->type, 1, 2, block, arena);
  inst->operands[inst->operands_len++] = operand;
  addOperandType(expr->expr_cast.type, inst, arena);

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irUnary(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand operand = irExpr(expr->expr_unary.arg, module, func, block, arena).operand;

  if (expr->expr_unary.op == TOKEN_NOT || expr->expr_unary.op == TOKEN_BIT_NOT) {
    struct IRInstruction *inst = newInst(OPCODE_BIT_XOR, expr->type, 1, 2, block, arena);
    inst->operands[inst->operands_len++] = operand;
    inst->operands[inst->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = expr->expr_unary.op == TOKEN_NOT ? 1 : -1
      }
    };

    return (struct IRExpr){
      .inst = inst,
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .ref = inst->vl
      }
    };
  }

  else if (expr->expr_unary.op == TOKEN_ASTERISK) {
    struct IRInstruction *inst = newInst(OPCODE_LOAD, expr->type, 1, 1, block, arena);
    inst->operands[inst->operands_len++] = operand;

    return (struct IRExpr){
      .inst = inst,
      .operand = (struct IROperand){
        .kind = Operand_Register,
        .ref = inst->vl
      }
    };
  }

  else if (expr->expr_unary.op == TOKEN_MINUS) {
    if (operand.constant.kind == Value_Int)
      operand.constant.i = -operand.constant.i;
    else if (operand.constant.kind == Value_Float)
      operand.constant.f = -operand.constant.f;
    else operand.constant.d = -operand.constant.d;
  }

  else if (expr->expr_unary.op == TOKEN_INCREMENT || expr->expr_unary.op == TOKEN_DECREMENT) {
    struct IRInstruction *inst = newInst(expr->expr_unary.op == TOKEN_INCREMENT ? OPCODE_ADD : OPCODE_SUB, expr->type, 1, 2, block, arena);
    inst->operands[inst->operands_len++] = operand;
    inst->operands[inst->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 1
      }
    };

    struct IROperand res = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    };

    if (!expr->expr_unary.prefix) {
      struct IRDef *def = getCurrentDef(func->ssa, &expr->expr_unary.arg->expr_identifier.name, block, arena);
      if (def->isParam)
        res.ref = def->param->vl;
      else res.ref = def->inst->vl;
    }

    if (expr->expr_unary.arg->expr_identifier.symbol->isGlobal) {
      struct IRInstruction *store = newInst(OPCODE_STORE, expr->type, 0, 2, block, arena);
      store->operands[store->operands_len++] = res;
      operand.kind = Operand_Global;
      store->operands[store->operands_len++] = operand;
      operand.kind = Operand_Register;
      return (struct IRExpr){0};
    } else setCurrentDef(func->ssa, &expr->expr_unary.arg->expr_identifier.name, 0, inst, block, arena);

    return (struct IRExpr){
      .inst = inst,
      .operand = res
    };
  }

  return (struct IRExpr){
    .operand = operand
  };
}

struct IRExpr irMember(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (expr->expr_member.member->expr_identifier.symbol->kind == Symbol_EnumValue)
    return irExpr(expr->expr_member.member->expr_identifier.symbol->symbol_enumvalue.expr, module, func, block, arena);

  struct IROperand obj = irExpr(expr->expr_member.obj, module, func, block, arena).operand;

  if (block->instructions[block->instructions_len - 1].opcode == OPCODE_LOAD) {
    (*block->reg)--;
    obj.ref = block->instructions[block->instructions_len - 1].operands[0].ref;
    block->instructions_len--;
  }

  struct IRType type = module->globals[expr->expr_member.obj->expr_identifier.symbol->index].value.type;
  struct IRInstruction *inst = newInst(OPCODE_GEP, NULL, 1, 3, block, arena);
  inst->type = type;
  inst->operands[inst->operands_len++] = obj;

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = expr->expr_member.member->expr_identifier.symbol->index
    }
  };

  struct IRInstruction *load = newInst(OPCODE_LOAD, expr->expr_member.member->expr_identifier.symbol->type, 1, 1, block, arena);
  addOperandRef(3, inst->vl, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = load->vl
    }
  };
}

struct IRExpr irIndex(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand refbase = irExpr(expr->expr_index.base, module, func, block, arena).operand;

  if (block->instructions[block->instructions_len - 1].opcode == OPCODE_LOAD) {
    (*block->reg)--;
    refbase.ref = block->instructions[block->instructions_len - 1].operands[0].ref;
    block->instructions_len--;
  }

  struct IROperand refindex = irExpr(expr->expr_index.index, module, func, block, arena).operand;
  struct IRInstruction *inst = newInst(OPCODE_GEP, expr->expr_index.base->type, 1, 3, block, arena);
  inst->operands[inst->operands_len++] = refbase;

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  inst->operands[inst->operands_len++] = refindex;
  struct Type *type = NULL;

  if (expr->expr_index.base->type->kind == Type_Pointer)
    type = expr->expr_index.base->type->type_pointer.base;
  else type = expr->expr_index.base->type->type_array.base;

  struct IRInstruction *load = newInst(OPCODE_LOAD, type, 1, 1, block, arena);
  addOperandRef(3, inst->vl, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = load->vl
    }
  };
}

struct IRExpr irCall(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IROperand operand = (struct IROperand){
    .kind = Operand_Call,
    .call = (struct IRCall){
      .operand = arena_alloc(arena, sizeof(struct IROperand)),
      .args = NULL,
      .args_len = 0
    }
  };

  if (expr->expr_call.callee->kind == Expr_Member) {
    *operand.call.operand = (struct IROperand){
      .kind = Operand_String,
      .string = &expr->expr_call.callee->expr_member.member->expr_identifier.symbol->name
    };

    operand.call.args = arena_alloc(arena, (expr->expr_call.args_len + 1) * sizeof(struct IROperand));
    operand.call.args_len++;
    operand.call.args[0] = (struct IROperand){
      .kind = Operand_Register,
      .ref = getCurrentDef(func->ssa, &expr->expr_call.callee->expr_member.obj->expr_identifier.name, block, arena)->inst->vl
    };
  } else {
    *operand.call.operand = irExpr(expr->expr_call.callee, module, func, block, arena).operand;
    operand.call.args = arena_alloc(arena, expr->expr_call.args_len * sizeof(struct IROperand));
  }

  for (int i = 0; i < expr->expr_call.args_len; i++) {
    struct IROperand ref = irExpr(expr->expr_call.args[i], module, func, block, arena).operand;
    operand.call.args[operand.call.args_len++] = ref;
  }

  if (cmpTP(expr->type, Primitive_Void)) {
    struct IRInstruction *inst = newInst(OPCODE_CALL, expr->type, 0, 1, block, arena);
    inst->operands[inst->operands_len++] = operand;
    return (struct IRExpr){0};
  }

  struct IRInstruction *inst = newInst(OPCODE_CALL, expr->type, 1, 1, block, arena);
  inst->operands[inst->operands_len++] = operand;

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irCallback(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct String *name = arena_alloc(arena, sizeof(struct String));
  int len = 7 + getNumCase(module->functions_len);
  char *buffer = arena_alloc(arena, len);
  snprintf(buffer, len, "lambda%ld", module->functions_len);
  *name = (struct String){
    .start = buffer,
    .length = len
  };

  struct IRFunction *f = newFunc(name, expr->expr_callback.params_len, &expr->expr_callback.params, module, arena);
  irStmt(expr->expr_callback.body, module, f, &f->blocks[0], arena);

  if (!f->blocks[f->blocks_len - 1].term) {
    struct IRInstruction *inst = newInst(OPCODE_RET, NULL, 0, 0, &f->blocks[f->blocks_len - 1], arena);
    f->blocks[f->blocks_len - 1].term = 1;
  }

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_String,
      .string = name
    }
  };
}

struct IRExpr irStructExpr(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRType type = module->globals[expr->expr_struct.symbol->index].value.type;
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, NULL, 1, 0, block, arena);
  inst->type = type;

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct IRInstruction *field = newInst(OPCODE_GEP, NULL, 1, 3, block, arena);
    field->type = type;
    addOperandRef(3, inst->vl, field);

    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = expr->expr_struct.fields[i].index
      }
    };

    struct IROperand operand = irExpr(expr->expr_struct.fields[i].expr, module, func, block, arena).operand;
    struct IRInstruction *store = newInst(OPCODE_STORE, expr->expr_struct.fields[i].expr->type, 0, 2, block, arena);
    store->operands[store->operands_len++] = operand;
    addOperandRef(3, field->vl, store);
  }

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

void fillArray(int64_t vl, struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < expr->expr_array.exprs_len; i++) {
    struct IRInstruction *gep = newInst(OPCODE_GEP, expr->type, 1, 3, block, arena);
    addOperandRef(3, vl, gep);

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = i
      }
    };

    if (expr->expr_array.exprs[i]->kind != Expr_Array) {
      struct IROperand operand = irExpr(expr->expr_array.exprs[i], module, func, block, arena).operand;
      struct IRInstruction *store = newInst(OPCODE_STORE, expr->expr_array.exprs[i]->type, 0, 2, block, arena);
      store->operands[store->operands_len++] = operand;
      addOperandRef(3, gep->vl, store);
    } else fillArray(gep->vl, expr->expr_array.exprs[i], module, func, block, arena);
  }
}

struct IRExpr irArray(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, expr->type, 1, 0, block, arena); 
  fillArray(inst->vl, expr, module, func, block, arena);
  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irId(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct Symbol *symbol = expr->expr_identifier.symbol;
  int64_t vl;

  if (symbol->kind == Symbol_Function) {
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_String,
        .string = &symbol->name
      }
    };
  }

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

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = vl
    }
  };
}

struct IRExpr irThis(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRInstruction *inst = newInst(OPCODE_GEP, NULL, 1, 3, block, arena);
  inst->type = module->globals[expr->expr_this.symbol->index].value.type;
  addOperandRef(3, 0, inst);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  struct Symbol *symbol = hashmap_get(expr->expr_this.symbol->symbol_struct.items, &expr->expr_this.str);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = symbol->index
    }
  };

  struct IRInstruction *load = newInst(OPCODE_LOAD, symbol->type, 1, 1, block, arena);
  addOperandRef(3, inst->vl, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = load->vl
    }
  };
}

struct IRExpr irLiteral(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (expr->expr_literal.kind == LITERAL_STRING) {
    if (module->globals_len == module->globals_cap) {
      size_t oldCap = module->globals_cap;
      module->globals_cap *= 2;
      struct IRGlobal *temp = arena_alloc(arena, module->globals_cap * sizeof(struct IRGlobal));
      memcpy(temp, module->globals, oldCap * sizeof(struct IRGlobal));
      module->globals = temp;
    }

    struct IRType *typebase = arena_alloc(arena, sizeof(struct IRType));
    typebase->kind = IRTYPEK_OTHER;
    typebase->other = IRTYPE_i8;
    struct IRType type;
    type.kind = IRTYPEK_ARRAY;
    type.array.type = typebase;
    type.array.size = expr->expr_literal.literal.string.length + 1;

    struct IRGlobal global = (struct IRGlobal){
      .vl = module->globals_len,
      .type = type,
      .value = (struct IROperand){
        .kind = Operand_Constant,
        .constant = (struct IRValue){
          .kind = Value_String,
          .s = &expr->expr_literal.literal.string
        }
      }
    };

    module->globals[module->globals_len++] = global;
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_Global,
        .ref = global.vl
      }
    };
  }

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Constant,
      .constant = irNumber(expr)
    }
  };
}

struct IRExpr irExpr(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRExpr irExpr = {0};

  if (expr->kind == Expr_Assign) irExpr = irAssign(expr, module, func, block, arena);
  else if (expr->kind == Expr_Logical) irExpr = irLogical(expr, module, func, block, arena);
  else if (expr->kind == Expr_Compare) irExpr = irCompare(expr, module, func, block, arena);
  else if (expr->kind == Expr_Binary) irExpr = irBinary(expr, module, func, block, arena);
  else if (expr->kind == Expr_Cast) irExpr = irCast(expr, module, func, block, arena);
  else if (expr->kind == Expr_Unary) irExpr = irUnary(expr, module, func, block, arena);
  else if (expr->kind == Expr_Member) irExpr = irMember(expr, module, func, block, arena);
  else if (expr->kind == Expr_Index) irExpr = irIndex(expr, module, func, block, arena);
  else if (expr->kind == Expr_Call) irExpr = irCall(expr, module, func, block, arena);
  else if (expr->kind == Expr_Callback) irExpr = irCallback(expr, module, func, block, arena);
  else if (expr->kind == Expr_Struct) irExpr = irStructExpr(expr, module, func, block, arena);
  else if (expr->kind == Expr_Array) irExpr = irArray(expr, module, func, block, arena);
  else if (expr->kind == Expr_Identifier) irExpr = irId(expr, module, func, block, arena);
  else if (expr->kind == Expr_This) irExpr = irThis(expr, module, func, block, arena);
  else if (expr->kind == Expr_Literal) irExpr = irLiteral(expr, module, func, block, arena);

  if (irExpr.block == NULL) irExpr.block = block;
  return irExpr;
}
