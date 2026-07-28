#include "../semantic.h"
#include "./expr/expr.h"
#include <utils/error.h>

void typeVar(struct SymbolTable *table, struct Decl *decl) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];
    table->scope->varType = var->type;
    if (var->expr != NULL) {
      struct Type *ret = typeExpr(table, var->expr);

      if (var->type->kind == Type_Auto) {
        struct Symbol *lvalue = findSymbol(table->scope, &var->name);
        lvalue->type = ret;
        var->type = ret;
      }

      if (!canImplicitConvert(table, ret, var->type)) {
        struct String s1 = getType(ret, table->program->arena);
        struct String s2 = getType(var->type, table->program->arena);
        errorLang(table->program->filename, decl->line, decl->column, "type '%.*s' is not assignable to type '%.*s'",
            s1.length, s1.start, s2.length, s2.start);
      }
    }
  }
}

void typeDecl(struct SymbolTable *table, struct Decl *decl) {
  if (decl->kind == Decl_Variable) typeVar(table, decl);
}
