#include "ir.h"
#include <stdio.h>
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
      .vl = module->globals_len++,
      .type = resolveTypeIR(var->type, arena),
      .value = (struct IROperand){
        .kind = Operand_Constant,
        .constant = 0
      }
    };

    if (var->expr != NULL) {
      if (var->expr->kind == Expr_Literal && var->expr->expr_literal.kind == LITERAL_STRING && var->symbol->type->kind != Type_Array) {
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
        type.array.size = var->expr->expr_literal.literal.string.length + 1;

        struct IRGlobal global2 = (struct IRGlobal){
          .vl = module->globals_len,
          .type = type,
          .value = (struct IROperand){
            .kind = Operand_Constant,
            .constant = (struct IRValue){
              .kind = Value_String,
              .s = &var->expr->expr_literal.literal.string
            }
          }
        };

        module->globals[module->globals_len++] = global2;
        global.value = (struct IROperand){
          .kind = Operand_Global,
          .ref = global2.vl
        };
      } else global.value.constant = irValueGlobal(global.vl, var->expr, module, arena);
    }

    var->symbol->isGlobal = 1;
    var->symbol->index = global.vl;
    module->globals[global.vl] = global;
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

void irStruct(struct Decl *decl, struct IRModule *module, struct Arena *arena) {
  if (module->globals_len == module->globals_cap) {
    size_t oldCap = module->globals_cap;
    module->globals_cap *= 2;
    struct IRGlobal *temp = arena_alloc(arena, module->globals_cap * sizeof(struct IRGlobal));
    memcpy(temp, module->globals, oldCap * sizeof(struct IRGlobal));
    module->globals = temp;
  }

  struct IRType type;
  type.kind = IRTYPEK_STRUCT;
  type.struct_.types = arena_alloc(arena, decl->decl_struct.fields_len * sizeof(struct IRType));
  type.struct_.types_len = decl->decl_struct.fields_len;

  for (int i = 0; i < type.struct_.types_len; i++)
    type.struct_.types[i] = resolveTypeIR(decl->decl_struct.fields[i].type, arena);

  struct IRGlobal global = (struct IRGlobal){
    .vl = module->globals_len++,
    .type = resolveTypeIR(NULL, arena),
    .value = (struct IROperand){
      .kind = Operand_Type,
      .type = type
    }
  };
  
  decl->decl_struct.symbol->isGlobal = 1;
  decl->decl_struct.symbol->index = global.vl;
  module->globals[global.vl] = global;

  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    struct String *name = arena_alloc(arena, sizeof(struct String));
    int len = method->name.length + getNumCase(module->functions_len) + 1;
    char *buffer = arena_alloc(arena, len);
    snprintf(buffer, len, "%.*s%ld", method->name.length, method->name.start, module->functions_len);
    *name = (struct String){
      .start = buffer,
      .length = len
    };
    struct IRFunction *func = newFunc(name, 0, &method->params, module, arena);
    struct Symbol *symbol = hashmap_get(decl->decl_struct.symbol->symbol_struct.items, &method->name);
    symbol->name = *name;
    func->params_len = method->params_len + 1;
    func->params = arena_alloc(arena, func->params_len * sizeof(struct IRParameter));
    func->params[0].type = (struct IRType){
      .kind = IRTYPEK_OTHER,
      .other = IRTYPE_ptr
    };
    func->params[0].vl = func->reg++;
    newDef(decl->decl_struct.symbol, func, arena);
    setCurrentDef(func->ssa, &decl->decl_struct.symbol->name, 1, &func->params[0], &func->blocks[0], arena);
    for (int j = 1; j < func->params_len; j++) {
      func->params[j].type = resolveTypeIR(method->params[j].type, arena);
      func->params[j].vl = func->reg++;
      newDef(method->params[j].symbol, func, arena);
      setCurrentDef(func->ssa, &method->params[j].name, 1, &func->params[j], &func->blocks[0], arena);
    }

    irStmt(method->body, module, func, &func->blocks[0], arena);
    if (!func->blocks[func->blocks_len - 1].term) {
      struct IRInstruction *inst = newInst(OPCODE_RET, NULL, 0, 0, &func->blocks[func->blocks_len - 1], arena);
      func->blocks[func->blocks_len - 1].term = 1;
    }
  }
}

void irGlobal(struct Decl *decl, struct IRModule *module, struct Arena *arena) {
  if (decl->kind == Decl_Variable) irVar(decl, module, arena);
  else if (decl->kind == Decl_Function) irFunc(decl, module, arena);
  else if (decl->kind == Decl_Public) irGlobal(decl->decl_public, module, arena);
  else if (decl->kind == Decl_Struct) irStruct(decl, module, arena);
}
