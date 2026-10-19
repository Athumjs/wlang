#include "ir.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

struct IRType resolveTypeIR(struct Type *type, struct Arena *arena) {
  struct IRType t = {0};

  if (type == NULL || type->kind == Type_Auto)
    t.kind = IRTYPEK_NULL;

  else if (type->kind == Type_Primitive) {
    t.kind = IRTYPEK_OTHER;

    if (cmpTP(type, Primitive_Void))
      t.other = IRTYPE_void;
    else if (cmpTP(type, Primitive_Boolean))
      t.other = IRTYPE_i1;
    else if (cmpTP(type, Primitive_Char))
      t.other = IRTYPE_i8;
    else if (cmpTP(type, Primitive_Byte))
      t.other = IRTYPE_i8;
    else if (cmpTP(type, Primitive_Short))
      t.other = IRTYPE_i16;
    else if (cmpTP(type, Primitive_Integer))
      t.other = IRTYPE_i32;
    else if (cmpTP(type, Primitive_Long))
      t.other = IRTYPE_i64;
    else if (cmpTP(type, Primitive_Float))
      t.other = IRTYPE_f32;
    else if (cmpTP(type, Primitive_Double))
      t.other = IRTYPE_f64;
  }

  else if (type->kind == Type_Pointer || type->kind == Type_Function) {
    t.kind = IRTYPEK_OTHER;
    t.other = IRTYPE_ptr;
  }

  else if (type->kind == Type_Array) {
    t.kind = IRTYPEK_ARRAY;
    t.array.type = arena_alloc(arena, sizeof(struct IRType));
    *t.array.type = resolveTypeIR(type->type_array.base, arena);
    t.array.size = type->type_array.expr_resolved;
  }

  return t;
}

void resolvePhi(struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, block, arena);
    uint8_t equal = 1;

    for (int j = 0; j < block->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, block->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue;
      equal = 0;
      break;
    }

    if (equal) continue;
    struct IRInstruction *inst = newInst(OPCODE_PHI, NULL, 1, block->prevs_len, block, arena);
    for (int j = 0; j < block->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, block->prevs[j], arena);
      if (pdef->isParam) addOperandPhi(pdef->param->vl, pdef->block, inst);
      else addOperandPhi(pdef->inst->vl, pdef->block, inst);
    }
    setCurrentDef(func->ssa, &func->defs[i]->name, 0, inst, block, arena);
  }
}

void resolvePhiIncompletes(struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena, struct IRInstruction ***phis) {
  for (int i = 0; i < func->defs_len; i++) {
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, block, arena);
    uint8_t equal = 1;

    for (int j = 0; j < block->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, block->prevs[j], arena);
      if (pdef == NULL) break;
      if (pdef == def) continue;
      equal = 0;
      break;
    }

    if (equal) continue;
    struct IRInstruction *phi = (*phis)[i];
    for (int j = 0; j < block->prevs_len; j++) {
      struct IRDef *pdef = getCurrentDef(func->ssa, &func->defs[i]->name, block->prevs[j], arena);
      if (pdef->isParam) phi->operands[phi->operands_len - 1].phi.value = pdef->param->vl;
      else phi->operands[phi->operands_len - 1].phi.value = pdef->inst->vl;
      phi->operands[phi->operands_len - 1].phi.block = pdef->block;
    }
  }
}

void addPhiImcompletes(struct IRFunction *func, struct IRBasicBlock *prevBlock, struct IRBasicBlock *block, struct IRBasicBlock *nextBlock, struct Arena *arena, struct IRInstruction ***phis) {
  for (int i = 0; i < func->defs_len; i++) {
    struct IRInstruction *inst = newInst(OPCODE_PHI, NULL, 1, block->prevs_len, block, arena);
    struct IRDef *def = getCurrentDef(func->ssa, &func->defs[i]->name, prevBlock, arena);
    if (def->isParam) addOperandPhi(def->param->vl, def->block, inst);
    else addOperandPhi(def->inst->vl, def->block, inst);
    addOperandPhi(-1, nextBlock, inst);
    setCurrentDef(func->ssa, &func->defs[i]->name, 0, inst, block, arena);
    (*phis)[i] = inst;
  }
}

struct IRDef *setCurrentDef(struct Hashmap *hmap, struct String *str, uint8_t isParam, void *ptr, struct IRBasicBlock *block, struct Arena *arena) {
  struct String name = getName(str, block->index, arena);
  struct IRDef *def = arena_alloc(arena, sizeof(struct IRDef));
  *def = (struct IRDef){
    .isParam = isParam,
    .block = block
  };

  if (isParam)
    def->param = ptr;
  else def->inst = ptr;

  hashmap_set(hmap, &name, def, arena);
  return def;
}

struct IRDef *getCurrentDef(struct Hashmap *hmap, struct String *str, struct IRBasicBlock *block, struct Arena *arena) {
  struct String name = getName(str, block->index, arena);
  struct IRDef *def = hashmap_get(hmap, &name);

  if (def == NULL) {
    if (block->prevs_len == 0) return def;

    if (block->prevs_len == 1)
      return getCurrentDef(hmap, str, block->prevs[0], arena);

    if (block->term) return def;

    struct IRInstruction *inst = NULL;

    for (int i = 0; i < block->prevs_len; i++) {
      struct IRDef *pdef = getCurrentDef(hmap, str, block->prevs[i], arena);
      if (pdef == NULL) return NULL;

      if (!pdef->isParam && pdef->inst->opcode == OPCODE_ALLOCA)
        return pdef;

      if (i == 0)
        inst = newInst(OPCODE_PHI, NULL, 1, block->prevs_len, block, arena);

      if (pdef->isParam) addOperandPhi(pdef->param->vl, pdef->block, inst);
      else addOperandPhi(pdef->inst->vl, pdef->block, inst);
    }

    return setCurrentDef(hmap, str, 0, inst, block, arena);
  }

  return def;
}

int getNumCase(int num) {
  if (num <= 0) return 1;
  return (int)log10(num) + 1;
}

struct String getName(struct String *str, int block_index, struct Arena *arena) {
  size_t len = str->length + 1 + getNumCase(block_index);
  char *buff = arena_alloc(arena, len);
  snprintf(buff, len + 1, "%.*s:%d", str->length, str->start, block_index);

  return (struct String){
    .length = len,
    .start = buff
  };
}

struct IRValue irNumber(struct Expr *expr) {
  struct IRValue value;

  if (expr->expr_literal.kind == LITERAL_INTEGER || expr->expr_literal.kind == LITERAL_BOOLEAN) {
    value.kind = Value_Int;
    value.i = expr->expr_literal.literal.numInt;
  }

  else if (expr->expr_literal.kind == LITERAL_UINTEGER) {
    value.kind = Value_UInt;
    value.u = expr->expr_literal.literal.numUint;
  }

  else if (expr->expr_literal.kind == LITERAL_CHAR) {
    value.kind = Value_Char;
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

struct IRValue irValueGlobal(int64_t id, struct Expr *expr, struct IRModule *module, struct Arena *arena) {
  if (expr->kind == Expr_Literal) {
    if (expr->expr_literal.kind == LITERAL_STRING) {
      if (expr->type->kind == Type_Array) {
        return (struct IRValue){
          .kind = Value_String,
          .s = &expr->expr_literal.literal.string
        };
      }
    }

    return irNumber(expr);
  }

  if (module->init.blocks_len == 0)
    newBlock(&module->init, arena);

  if (module->init.blocks[0].term)
    module->init.blocks[0].instructions_len--;

  struct IROperand ref = irExpr(expr, module, &module->init, &module->init.blocks[0], arena).operand;
  struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, &module->init.blocks[0], arena);
  inst->operands[inst->operands_len++] = ref;
  addOperandRef(1, id, inst);
  newInst(OPCODE_RET, NULL, 0, 0, &module->init.blocks[0], arena);
  module->init.blocks[0].term = 1;
  return (struct IRValue){
    .kind = Value_Int,
    .i = 0
  };
}
