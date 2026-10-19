#include "ir.h"

void irVarLocal(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];

    if (var->expr != NULL) {
      struct IRExpr ref = irExpr(var->expr, module, func, block, arena);

      if (var->symbol->isPtr) {
        if (block->instructions[block->instructions_len - 1].opcode == OPCODE_LOAD) {
          (*block->reg)--;
          ref.operand.ref = block->instructions[block->instructions_len - 1].operands[0].ref;
          block->instructions_len--;
        }

        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, var->symbol->type, 1, 0, block, arena);
        struct IRInstruction *store = newInst(OPCODE_STORE, var->symbol->type, 0, 2, block, arena);
        store->operands[store->operands_len++] = ref.operand;
        addOperandRef(3, inst->vl, store);
        newDef(var->symbol, func, arena);
        setCurrentDef(func->ssa, &var->name, 0, inst, block, arena);
        continue;
      }
      
      if (ref.operand.kind == Operand_Constant) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, var->symbol->type, 1, 1, block, arena);
        inst->operands[inst->operands_len++] = ref.operand;
        ref.inst = inst;
      }

      if (var->expr->kind == Expr_Literal && var->expr->expr_literal.kind == LITERAL_STRING) {
        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, var->symbol->type, 1, 0, block, arena);
        
        if (var->symbol->type->kind == Type_Array) {
          struct IRInstruction *memcpy = newInst(OPCODE_MEMCPY, NULL, 0, 3, block, arena);
          addOperandRef(3, inst->vl, memcpy);
          memcpy->operands[memcpy->operands_len++] = ref.operand;

          memcpy->operands[memcpy->operands_len++] = (struct IROperand){
            .kind = Operand_Constant,
            .constant = (struct IRValue){
              .kind = Value_Int,
              .i = module->globals[ref.operand.ref].type.array.size
            }
          };
        } else {
          struct IRInstruction *store = newInst(OPCODE_STORE, var->symbol->type, 0, 2, block, arena);
          store->operands[store->operands_len++] = ref.operand;
          addOperandRef(3, inst->vl, store);
        }

        newDef(var->symbol, func, arena);
        setCurrentDef(func->ssa, &var->name, 0, inst, block, arena);
      } else {
        newDef(var->symbol, func, arena);
        setCurrentDef(func->ssa, &var->name, 0, ref.inst, block, arena);
      }

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
