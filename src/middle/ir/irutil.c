#include "ir.h"
#include <math.h>
#include <stdio.h>

void setCurrentDef(struct Hashmap *hmap, struct String *str, uint8_t isParam, void *ptr, int64_t index, struct Arena *arena) {
  struct String name = getName(str, index, arena);
  struct IRDef *def = arena_alloc(arena, sizeof(struct IRDef));
  *def = (struct IRDef){
    .isParam = isParam,
  };

  if (isParam)
    def->param = ptr;
  else def->inst = ptr;

  hashmap_set(hmap, &name, def, arena);
}

struct IRDef *getCurrentDef(struct Hashmap *hmap, struct String *str, struct IRBasicBlock *block, struct Arena *arena) {
  struct String name = getName(str, block->index, arena);
  struct IRDef *def = hashmap_get(hmap, &name);

  if (def == NULL) {
    if (block->prevs_len != 1) return NULL;
    return getCurrentDef(hmap, str, block->prevs[0], arena);
  } else def->block = block;

  return def;
}

static int getNumCase(int num) {
  if (num <= 0) return 1;
  return (int)log10(num) + 1;
}

struct String getName(struct String *str, int block_index, struct Arena *arena) {
  size_t len = str->length + 1 + getNumCase(block_index);
  char *buff = arena_alloc(arena, len);
  snprintf(buff, sizeof(buff), "%.*s:%d", str->length, str->start, block_index);

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
  if (expr->kind == Expr_Literal)
    return irNumber(expr);

  if (module->init.blocks_len == 0)
    newBlock(&module->init, arena);

  struct IROperand ref = irExpr(expr, module, &module->init, &module->init.blocks[0], arena);
  struct IRInstruction *inst = newInst(OPCODE_STORE, expr->type, 0, 2, &module->init.blocks[0], arena);
  inst->operands[inst->operands_len++] = ref;
  addOperandRef(1, id, inst);
  newInst(OPCODE_RET, NULL, 0, 0, &module->init.blocks[0], arena);

  return (struct IRValue){
    .kind = Value_Int,
    .i = 0
  };
}
