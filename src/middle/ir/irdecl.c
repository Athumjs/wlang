#include "ir.h"

void irVarLocal(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];
    newDef(var->symbol, func, arena);
    setCurrentDef(func->ssa, &var->name, 0, &block->instructions[block->instructions_len], block->index, arena);

    if (var->expr != NULL) {
      struct IROperand ref = irExpr(var->expr, module, func, block, arena);

      if (var->expr->kind == Expr_Literal) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, var->symbol->type, 1, 1, block, arena);
        inst->operands[inst->operands_len++] = ref;
      }

      continue;
    }

    newInst(OPCODE_ALLOCA, var->symbol->type, 1, 0, block, arena);
  }
}

void irDecl(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (decl->kind == Decl_Variable) irVarLocal(decl, module, func, block, arena);
}
