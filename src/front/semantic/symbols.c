#include "semantic.h"
#include <utils/error.h>
#include <utils/hashmap.h>

struct Symbol *newSymbol(struct SymbolTable *table, enum SymbolKind k, int l, int c, struct String n, struct Type *t) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = k;
  symbol->line = l;
  symbol->column = c;
  symbol->name = n;
  symbol->type = t;
  return symbol;
}

void addSymbolVar(struct SymbolTable *table, struct Var *var, uint8_t isConst, uint8_t isPublic) {
  resolveType(table, &var->type);
  struct Symbol *symbol = newSymbol(table, Symbol_Variable, var->line, var->column, var->name, var->type);
  symbol->symbol_variable.isConst = isConst;
  addSymbol(table, symbol);
  if (isPublic) hashmap_set(table->exports, &var->name, symbol, table->program->arena);
  var->symbol = symbol;
}

void varSymbol(struct SymbolTable *table, struct Decl *decl, uint8_t isPublic) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];
    addSymbolVar(table, var, decl->decl_variable.isConst, isPublic);
    table->scope->varType = var->type;
    if (var->expr != NULL) resolveExpr(table, var->expr);
  }
}

struct Type *newTypeFunc(struct SymbolTable *table, struct Type **t, int pl, struct Param *ps) {
  struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
  type->kind = Type_Function;
  resolveType(table, t);
  type->type_function.retType = *t;
  type->type_function.params_len = pl;
  type->type_function.params = arena_alloc(table->program->arena, type->type_function.params_len * sizeof(struct Type *));
  for (int i = 0; i < type->type_function.params_len; i++) {
    resolveType(table, &ps[i].type);
    type->type_function.params[i] = ps[i].type;
  }
  return type;
}

void funcSymbol(struct SymbolTable *table, struct Decl *decl, uint8_t isPublic) {
  if (table->scope->prev != NULL) {
    errorLang(table->program->filename, decl->line, decl->column, "function definition is not allowed here");
  }

  struct Type *type = newTypeFunc(table, &decl->decl_function.retType, decl->decl_function.params_len,
      decl->decl_function.params);
  struct Symbol *symbol = newSymbol(table, Symbol_Function, decl->line, decl->column, decl->decl_function.name, type);
  symbol->symbol_function.params = decl->decl_function.params;
  symbol->symbol_function.params_len = decl->decl_function.params_len;
  addSymbol(table, symbol);
  if (isPublic) hashmap_set(table->exports, &decl->decl_function.name, symbol, table->program->arena);
}

struct Type *newTypeNamed(struct SymbolTable *table, struct String name) {
  struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
  type->kind = Type_Named;
  type->type_named.name = name;
  return type;
}

void enumSymbol(struct SymbolTable *table, struct Decl *decl, uint8_t isPublic) {
  struct Type *type = newTypeNamed(table, decl->decl_enum.name);
  struct Symbol *symbol = newSymbol(table, Symbol_Enum, decl->line, decl->column, decl->decl_enum.name, type);
  symbol->symbol_enum.items = hashmap_new(table->program->arena, 8);
  for (int i = 0; i < decl->decl_enum.elems_len; i++) {
    struct Element *elem = &decl->decl_enum.elems[i];
    struct Symbol *eSymbol = newSymbol(table, Symbol_Enum, elem->line, elem->column, elem->name, symbol->type);
    hashmap_set(symbol->symbol_enum.items, &elem->name, eSymbol, table->program->arena);
  }
  addSymbol(table, symbol);
  if (isPublic) hashmap_set(table->exports, &decl->decl_enum.name, symbol, table->program->arena);
}

void structSymbol(struct SymbolTable *table, struct Decl *decl, uint8_t isPublic) {
  struct Type *type = newTypeNamed(table, decl->decl_struct.name);
  struct Symbol *symbol = newSymbol(table, Symbol_Struct, decl->line, decl->column, decl->decl_struct.name, type);
  symbol->symbol_struct.items = hashmap_new(table->program->arena, 8);
  for (int i = 0; i < decl->decl_struct.fields_len; i++) {
    struct Field *field = &decl->decl_struct.fields[i];
    resolveType(table, &field->type);
    struct Symbol *fSymbol = newSymbol(table, Symbol_Variable, field->line, field->column, field->name, field->type);
    fSymbol->symbol_variable.isConst = 0;
    hashmap_set(symbol->symbol_struct.items, &field->name, fSymbol, table->program->arena);
  }
  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    struct Type *type = newTypeFunc(table, &method->retType, method->params_len, method->params);
    struct Symbol *mSymbol = newSymbol(table, Symbol_Function, method->line, method->column, method->name, type);
    mSymbol->symbol_function.params = method->params;
    mSymbol->symbol_function.params_len = method->params_len;
    hashmap_set(symbol->symbol_struct.items, &method->name, mSymbol, table->program->arena);
  }
  addSymbol(table, symbol);
  if (isPublic) hashmap_set(table->exports, &decl->decl_struct.name, symbol, table->program->arena);
}

void resolveSymbols(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];
    if (decl->kind == Decl_Public) {
      decl = decl->decl_public;
      if (decl->kind == Decl_Variable) varSymbol(table, decl, 1);
      else if (decl->kind == Decl_Function) funcSymbol(table, decl, 1);
      else if (decl->kind == Decl_Enum) enumSymbol(table, decl, 1);
      else if (decl->kind == Decl_Struct) structSymbol(table, decl, 1);
    }

    else if (decl->kind == Decl_Variable) varSymbol(table, decl, 0);
    else if (decl->kind == Decl_Function) funcSymbol(table, decl, 0);
    else if (decl->kind == Decl_Enum) enumSymbol(table, decl, 0);
    else if (decl->kind == Decl_Struct) structSymbol(table, decl, 0);
    continue;
  }
}

void resolveDecl(struct SymbolTable *table, struct Decl *decl) {
  if (decl->kind == Decl_Variable) varSymbol(table, decl, 0);
  else if (decl->kind == Decl_Function) funcSymbol(table, decl, 0);
  else if (decl->kind == Decl_Enum) enumSymbol(table, decl, 0);
  else if (decl->kind == Decl_Struct) structSymbol(table, decl, 0);
  else errorLang(table->program->filename, decl->line, decl->column, "this declaration is not allowed in this scope");
}
