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
  param->symbol = symbol;
}

void resolveFunc(struct SymbolTable *table, struct Decl *decl) {
  struct Symbol *symbol = findSymbol(table->scope, &decl->decl_function.name);

  if (decl->decl_function.body->kind == Stmt_Block) {
    enterScope(table);
    for (int i = 0; i < decl->decl_function.params_len; i++) {
      addSymbolParam(table, &decl->decl_function.params[i], 0);
    }
    decl->decl_function.body->stmt_block.scope = table->scope;
    decl->decl_function.body->stmt_block.expectType = decl->decl_function.retType;
  }

  resolveStmt(table, decl->decl_function.body);
}

void resolveStruct(struct SymbolTable *table, struct Decl *decl) {
  struct Symbol *symbol = findSymbol(table->scope, &decl->decl_struct.name);
  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    table->scope->currentStruct = symbol;
    table->scope->expectType = method->retType;

    if (method->body->kind == Stmt_Block) {
      enterScope(table);
      for (int i = 0; i < method->params_len; i++) {
        addSymbolParam(table, &method->params[i], 0);
      }
      method->body->stmt_block.scope = table->scope;
      method->body->stmt_block.expectType = method->retType;
    }

    resolveStmt(table, method->body);
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
