#include "../semantic.h"

void typeVar(struct SymbolTable *table, struct Decl *decl) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];
    table->scope->varType = var->type;
    typeExpr(table, var->expr);
  }
}

void typeDecl(struct SymbolTable *table, struct Decl *decl) {
  if (decl->kind == Decl_Variable) typeVar(table, decl);
}
