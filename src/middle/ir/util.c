#include "ir.h"
#include <utils/numeric.h>
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

  else if (type->kind == Type_Named) {
    if (type->type_named.type == NULL)
      t.kind = IRTYPEK_NULL;
    else t = resolveTypeIR(type->type_named.type, arena);
  }

  return t;
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
  return def;
}

struct Phi {
  struct IRInstruction *inst;
  int64_t index;
};

struct Phi addPhi(size_t operands_len, struct IRBasicBlock *block, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  int index = 0;

  while (index < block->instructions_len && block->instructions[index].opcode == OPCODE_PHI)
    index++;

  if (index < block->instructions_len)
    memmove(&block->instructions[index + 1], &block->instructions[index], (block->instructions_len - index) * sizeof(struct IRInstruction));

  block->instructions[index] = (struct IRInstruction){
    .opcode = OPCODE_PHI,
    .vl = (*block->reg)++,
    .operands = arena_alloc(arena, operands_len * sizeof(struct IROperand)),
    .operands_len = 0
  };

  block->instructions_len++;
  return (struct Phi){
    .inst = &block->instructions[index],
    .index = index
  };
}

struct IRDef *readVariable(struct Hashmap *hmap, struct String *str, struct IRType type, struct IRBasicBlock *block, struct Arena *arena) {
  struct IRDef *def = getCurrentDef(hmap, str, block, arena);
  if (def != NULL) return def;

  if (!block->sealed) {
    struct Phi phi = addPhi(1, block, arena);
    struct IRInstruction *inst = phi.inst;
    inst->operands[0].op_type = type;
    struct IRDef *new_def = setCurrentDef(hmap, str, 0, inst, block, arena);
    if (block->imcomplete_phis_len == block->imcomplete_phis_cap) {
      size_t oldCap = block->imcomplete_phis_cap;
      block->imcomplete_phis_cap *= 2;
      struct IRImcompletePhi *temp = arena_alloc(arena, block->imcomplete_phis_cap * sizeof(struct IRImcompletePhi));
      memcpy(temp, block->imcomplete_phis, oldCap * sizeof(struct IRImcompletePhi));
      block->imcomplete_phis = temp;
    }
    block->imcomplete_phis[block->imcomplete_phis_len++] = (struct IRImcompletePhi){
      .name = str,
      .type = type,
      .index = phi.index
    };
    return new_def;
  }

  struct IRInstruction *inst = addPhi(block->prevs_len, block, arena).inst;
  struct IRDef *new_def = setCurrentDef(hmap, str, 0, inst, block, arena);

  if (block->prevs_len == 1) {
    struct IRDef *def = readVariable(hmap, str, type, block->prevs[0], arena);
    if (def->isParam)
      addOperandPhi(def->param->vl, def->param->type, block->prevs[0], inst);
    else addOperandPhi(def->inst->vl, type, block->prevs[0], inst);
    return new_def;
  }

  for (int i = 0; i < block->prevs_len; i++) {
    struct IRDef *def = readVariable(hmap, str, type, block->prevs[i], arena);
    if (def->isParam)
      addOperandPhi(def->param->vl, def->param->type, block->prevs[i], inst);
    else addOperandPhi(def->inst->vl, type, block->prevs[i], inst);
  }

  return new_def;
}

void sealBlock(struct Hashmap *hmap, struct IRBasicBlock *block, struct Arena *arena) {
  block->sealed = 1;

  for (int i = 0; i < block->imcomplete_phis_len; i++) {
    struct IRInstruction *phi = &block->instructions[block->imcomplete_phis[i].index];
    phi->operands = arena_alloc(arena, block->prevs_len * sizeof(struct IROperand));
    for (int j = 0; j < block->prevs_len; j++) {
      struct IRDef *def = readVariable(hmap, block->imcomplete_phis[i].name, block->imcomplete_phis[i].type, block->prevs[j], arena);
      if (def->isParam)
        addOperandPhi(def->param->vl, def->param->type, block->prevs[j], phi);
      else addOperandPhi(def->inst->vl, def->inst->operands[0].op_type, block->prevs[j], phi);
    }
  }
  block->imcomplete_phis_len = 0;
}

struct String getName(struct String *str, int block_index, struct Arena *arena) {
  size_t len = str->length + getNumCase(block_index) + 2;
  char *buff = arena_alloc(arena, len);
  snprintf(buff, len, "%.*s:%d", str->length, str->start, block_index);

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

struct IRValue irValueGlobal(struct Expr *expr, struct Arena *arena) {
  if (expr == NULL) {
    return (struct IRValue){
      .kind = Value_Int,
      .i = 0
    };
  }

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

  if (expr->kind == Expr_Array) {
    struct IRValue value = (struct IRValue){
      .kind = Value_Array,
      .a.elems = arena_alloc(arena, expr->expr_array.exprs_len * sizeof(struct IRValue)),
      .a.elems_len = expr->expr_array.exprs_len
    };

    for (int i = 0; i < expr->expr_array.exprs_len; i++)
      value.a.elems[i] = irValueGlobal(expr->expr_array.exprs[i], arena);

    return value;
  }

  struct IRValue value = {0};
  struct IRValue lvalue = irValueGlobal(expr->expr_binary.left, arena);
  struct IRValue rvalue = irValueGlobal(expr->expr_binary.right, arena);
  value.kind = lvalue.kind;

  if (expr->expr_binary.op == TOKEN_PLUS) {
    if (value.kind == Value_Int)
      value.i = lvalue.i + rvalue.i;
    else if (value.kind == Value_UInt)
      value.u = lvalue.u + rvalue.u;
    else if (value.kind == Value_Float)
      value.f = lvalue.f + rvalue.f;
    else value.d = lvalue.d + rvalue.d;
  }

  if (expr->expr_binary.op == TOKEN_MINUS) {
    if (value.kind == Value_Int)
      value.i = lvalue.i - rvalue.i;
    else if (value.kind == Value_UInt)
      value.u = lvalue.u - rvalue.u;
    else if (value.kind == Value_Float)
      value.f = lvalue.f - rvalue.f;
    else value.d = lvalue.d - rvalue.d;
  }

  if (expr->expr_binary.op == TOKEN_ASTERISK) {
    if (value.kind == Value_Int)
      value.i = lvalue.i * rvalue.i;
    else if (value.kind == Value_UInt)
      value.u = lvalue.u * rvalue.u;
    else if (value.kind == Value_Float)
      value.f = lvalue.f * rvalue.f;
    else value.d = lvalue.d * rvalue.d;
  }

  if (expr->expr_binary.op == TOKEN_SLASH) {
    if (value.kind == Value_Int)
      value.i = lvalue.i / rvalue.i;
    else if (value.kind == Value_UInt)
      value.u = lvalue.u / rvalue.u;
    else if (value.kind == Value_Float)
      value.f = lvalue.f / rvalue.f;
    else value.d = lvalue.d / rvalue.d;
  }

  if (expr->expr_binary.op == TOKEN_MOD) {
    if (value.kind == Value_Int)
      value.i = lvalue.i % rvalue.i;
    else if (value.kind == Value_UInt)
      value.u = lvalue.u % rvalue.u;
    else if (value.kind == Value_Float)
      value.f = fmodf(lvalue.f, rvalue.f);
    else value.d = fmod(lvalue.d, rvalue.d);
  }

  return value;
}
