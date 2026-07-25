#include "semantic.h"

void resolveFunc(struct SymbolTable *table, struct Decl *decl) {
  enterScope(table);

  for (int i = 0; i < decl->decl_function.params_len; i++) {
    addSymbolVar(table, (struct Var *)&decl->decl_function.params[i], 0);
  }

  resolveStmt(table, decl->decl_function.body);
  decl->decl_function.scope = table->scope;
  exitScope(table);
}

void resolveStruct(struct SymbolTable *table, struct Decl *decl) {
  struct Symbol *symbol = findSymbol(table->scope, &decl->decl_struct.name);
  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    enterScope(table);
    table->scope->currentStruct = symbol;
    table->scope->expectType = method->retType;

    for (int i = 0; i < method->params_len; i++) {
      addSymbolVar(table, (struct Var *)&method->params[i], 0);
    }

    resolveStmt(table, method->body);
    method->scope = table->scope;
    exitScope(table);
  }
}

void resolveScopes(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];
    if (decl->kind == Decl_Function) resolveFunc(table, decl);
    else if (decl->kind == Decl_Struct) resolveStruct(table, decl);
    continue;
  }
}
