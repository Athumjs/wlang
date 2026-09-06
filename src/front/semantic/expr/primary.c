#include "../semantic.h"
#include <utils/hashmap.h>
#include <utils/error.h>

static struct Symbol *newSymbol(struct SymbolTable *table, enum SymbolKind k, int l, int c, struct String n, struct Type *t) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = k;
  symbol->line = l;
  symbol->column = c;
  symbol->name = n;
  symbol->type = t;
  return symbol;
}

static void addSymbolParam(struct SymbolTable *table, struct Param *param, uint8_t isConst) {
  resolveType(table, &param->type);
  struct Symbol *symbol = newSymbol(table, Symbol_Variable, param->line, param->column, param->name, param->type);
  symbol->symbol_variable.isConst = isConst;
  addSymbol(table, symbol);
}

struct Symbol *resolveExprCallback(struct SymbolTable *table, struct Expr *expr) {
  resolveType(table, &expr->expr_callback.retType);

  if (expr->expr_callback.body->kind == Stmt_Block) {
    enterScope(table);
    for (int i = 0; i < expr->expr_callback.params_len; i++) {
      addSymbolParam(table, &expr->expr_callback.params[i], 0);
    }
    expr->expr_callback.body->stmt_block.scope = table->scope;
    expr->expr_callback.body->stmt_block.expectType = expr->expr_callback.retType;
  }

  resolveStmt(table, expr->expr_callback.body);
  return NULL;
}

struct Symbol *resolveExprStruct(struct SymbolTable *table, struct Expr *expr) {
  if (table->scope->varType->kind == Type_Auto) {
    errorLang(table->program->filename, expr->line, expr->column, "cannot infer type of struct literal");
  }

  if (table->scope->varType->kind != Type_Named) {
    errorLang(table->program->filename, expr->line, expr->column, "expected expression");
  }

  struct Symbol *type = findSymbol(table->scope, &table->scope->varType->type_named.name);

  if (type == NULL) {
    errorLang(table->program->filename, expr->line, expr->column, "cannot find name '%.*s'", 
        table->scope->varType->type_named.name.length, table->scope->varType->type_named.name.start);
  }

  if (type->kind != Symbol_Struct) {
    struct String str = getType(table->scope->varType, table->program->arena);
    errorLang(table->program->filename, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
  }

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct Field *f1 = &expr->expr_struct.fields[i];
    struct Symbol *f2 = hashmap_get(type->symbol_struct.items, &f1->name);

    if (f2 == NULL) {
      errorLang(table->program->filename, f1->line, f1->column, "cannot find name '%.*s'",
          f1->name.length, f1->name.start);
    }

    resolveExpr(table, expr->expr_struct.fields[i].expr);
  }
  return NULL;
}

struct Symbol *resolveExprArray(struct SymbolTable *table, struct Expr *expr) {
  for (int i = 0; i < expr->expr_array.exprs_len; i++) {
    resolveExpr(table, expr->expr_array.exprs[i]);
  }
  return NULL;
}

struct Symbol *resolveExprThis(struct SymbolTable *table, struct Expr *expr) {
  if (table->scope->currentStruct == NULL) {
    errorLang(table->program->filename, expr->line, expr->column, "'this' outside of struct method");
  }

  struct Symbol *member = hashmap_get(table->scope->currentStruct->symbol_struct.items, &expr->expr_this);
 
  if (member == NULL) {
    errorLang(table->program->filename, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
        expr->expr_this.length, expr->expr_this.start, table->scope->currentStruct->name.length, table->scope->currentStruct->name.start);
  }

  return member;
}

struct Symbol *resolveExprIdentifier(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *symbol = findSymbol(table->scope, &expr->expr_identifier.name);
  expr->expr_identifier.symbol = symbol;

  if (symbol == NULL || expr->line < symbol->line || (expr->line == symbol->line && expr->column < symbol->column)) {
    errorLang(table->program->filename, expr->line, expr->column, "cannot find name '%.*s'", expr->expr_identifier.name.length, expr->expr_identifier.name.start);
  }

  return symbol;
}
