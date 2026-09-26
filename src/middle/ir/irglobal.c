#include "ir.h"
#include <string.h>

void irVar(struct Decl *decl, struct IRModule *module, struct Arena *arena) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];
    if (module->globals_len == module->globals_cap) {
      size_t oldCap = module->globals_cap;
      module->globals_cap *= 2;
      struct IRGlobal *temp = arena_alloc(arena, module->globals_cap * sizeof(struct IRGlobal));
      memcpy(temp, module->globals, oldCap * sizeof(struct IRGlobal));
      module->globals = temp;
    }

    struct IRGlobal global = (struct IRGlobal){
      .vl = module->globals_len,
      .type = var->type,
      .value = (struct IROperand){
        .kind = Operand_Constant,
        .constant = 0
      }
    };

    if (var->expr != NULL)
      global.value.constant = irValueGlobal(module->globals_len, var->expr, module, arena);

    var->symbol->isGlobal = 1;
    var->symbol->index = module->globals_len;
    module->globals[module->globals_len++] = global;
  }
}

void irFunc(struct Decl *decl, struct IRModule *module, struct Arena *arena) {
  struct IRFunction *func = newFunc(&decl->decl_function.name, decl->decl_function.params_len, &decl->decl_function.params, module, arena);
  irStmt(decl->decl_function.body, module, func, &func->blocks[0], arena);

  if (!func->blocks[func->blocks_len - 1].term) {
    struct IRInstruction *inst = newInst(OPCODE_RET, NULL, 0, 0, &func->blocks[func->blocks_len - 1], arena);
    func->blocks[func->blocks_len - 1].term = 1;
  }
}

void irGlobal(struct Decl *decl, struct IRModule *module, struct Arena *arena) {
  if (decl->kind == Decl_Variable) irVar(decl, module, arena);
  else if (decl->kind == Decl_Function) irFunc(decl, module, arena);
}
