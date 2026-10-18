#include "ir.h"

void irVarLocal(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];

    if (var->expr != NULL) {
      struct IROperand ref = irExpr(var->expr, module, func, block, arena).operand;

      if (var->symbol->isPtr) {
        if (block->instructions[block->instructions_len - 1].opcode == OPCODE_LOAD) {
          block->instructions_len--;
          (*block->reg)--;
          ref.ref--;
        }

        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, var->symbol->type, 1, 0, block, arena);
        struct IRInstruction *store = newInst(OPCODE_STORE, var->symbol->type, 0, 2, block, arena);
        store->operands[store->operands_len++] = ref;
        addOperandRef(3, inst->vl, store);
        newDef(var->symbol, func, arena);
        setCurrentDef(func->ssa, &var->name, 0, inst, block, arena);
        continue;
      }
      
      if (ref.kind == Operand_Constant) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, var->symbol->type, 1, 1, block, arena);
        inst->operands[inst->operands_len++] = ref;
      }

      newDef(var->symbol, func, arena);
      setCurrentDef(func->ssa, &var->name, 0, &block->instructions[block->instructions_len - 1], block, arena);
      continue;
    }

    struct IRInstruction *inst = newInst(OPCODE_ALLOCA, var->symbol->type, 1, 0, block, arena);
    newDef(var->symbol, func, arena);
    setCurrentDef(func->ssa, &var->name, 0, inst, block, arena);
  }
}

void irDecl(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  if (decl->kind == Decl_Variable) irVarLocal(decl, module, func, block, arena);
}
