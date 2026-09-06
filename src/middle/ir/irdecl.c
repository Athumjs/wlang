#include "ir.h"
#include <string.h>

void irVar(struct Decl *decl, struct IRModule *ir, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    if (ir->globals_len == ir->globals_cap) {
      size_t oldCap = ir->globals_cap;
      ir->globals_cap *= 2;
      struct IRGlobal *temp = arena_alloc(arena, ir->globals_cap * sizeof(struct IRGlobal));
      memcpy(temp, ir->globals, oldCap * sizeof(struct IRGlobal));
      ir->globals = temp;
    }

    struct IRGlobal global = (struct IRGlobal){
      .result = ir->globals_len,
      .type = decl->decl_variable.vars[i].symbol->type
    };

    if (decl->decl_variable.vars[i].expr != NULL) {
      global.value = (struct IROperand){
        .kind = Operand_Constant,
        .constant = irValue(ir->globals_len, decl->decl_variable.vars[i].expr, ir, arena)
      };
    } else {
      global.value = (struct IROperand){
        .kind = Operand_Constant,
        .constant = 0
      };
    }

    ir->globals[ir->globals_len++] = global;
    decl->decl_variable.vars[i].symbol->ptr.kind = Pointer_Global;
    decl->decl_variable.vars[i].symbol->ptr.global = &ir->globals[ir->globals_len - 1];
  }
}

void irFunc(struct Decl *decl, struct IRModule *ir, struct Arena *arena) {
  if (ir->functions_len == ir->functions_cap) {
    size_t oldCap = ir->functions_cap;
    ir->functions_cap *= 2;
    struct IRFunction *temp = arena_alloc(arena, ir->functions_cap * sizeof(struct IRFunction));
    memcpy(temp, ir->functions, oldCap * sizeof(struct IRFunction));
    ir->functions = temp;
  }

  struct IRFunction func = {
    .name = &decl->decl_function.name,
    .blocks_cap = 2,
    .blocks = arena_alloc(arena, 2 * sizeof(struct IRBasicBlock)),
    .blocks_len = 1,
    .regs_len = 0
  };

  struct IRBasicBlock block = {
    .instructions_cap = 8,
    .instructions = arena_alloc(arena, 8 * sizeof(struct IRInstruction)),
    .instructions_len = 0
  };

  func.blocks[0] = block;
  ir->functions[ir->functions_len++] = func;
  irStmt(decl->decl_function.body, ir, arena);
}

void irDecl(struct Decl *decl, struct IRModule *ir, struct Arena *arena) {
  if (decl->kind == Decl_Variable) irVar(decl, ir, arena);
  else if (decl->kind == Decl_Function) irFunc(decl, ir, arena);
}
