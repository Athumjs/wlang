#include "semantic.h"

static struct Symbol *newSymbol(struct SymbolTable *table, enum SymbolKind k, int l, int c, struct String n, struct Type *t) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = k;
  symbol->line = l;
  symbol->column = c;
  symbol->name = n;
  symbol->type = t;
  return symbol;
}

void addSymbolParam(struct SymbolTable *table, struct Param *param, uint8_t isConst) {
  resolveType(table, &param->type);
  struct Symbol *symbol = newSymbol(table, Symbol_Variable, param->line, param->column, param->name, param->type);
  symbol->symbol_variable.isConst = isConst;
  addSymbol(table, symbol);
}

void resolveFunc(struct SymbolTable *table, struct Decl *decl) {
  struct Symbol *symbol = findSymbol(table->scope, &decl->decl_function.name);
  enterScope(table);

  for (int i = 0; i < decl->decl_function.params_len; i++) {
    addSymbolParam(table, &decl->decl_function.params[i], 0);
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
      addSymbolParam(table, &method->params[i], 0);
    }

    resolveStmt(table, method->body);
    method->scope = table->scope;
    exitScope(table);
  }
}

void resolveScopes(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];
    if (decl->kind == Decl_Public) {
      decl = decl->decl_public;
      if (decl->kind == Decl_Function) resolveFunc(table, decl);
      else if (decl->kind == Decl_Struct) resolveStruct(table, decl);
    }

    else if (decl->kind == Decl_Function) resolveFunc(table, decl);
    else if (decl->kind == Decl_Struct) resolveStruct(table, decl);
    continue;
  }
}
