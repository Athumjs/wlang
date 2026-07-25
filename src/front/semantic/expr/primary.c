#include "../semantic.h"
#include <utils/hashmap.h>
#include <utils/error.h>

struct Symbol *resolveExprStruct(struct SymbolTable *table, struct Expr *expr) {
  if (table->scope->varType->kind == Type_Auto) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "cannot infer type of struct literal");
  }

  if (table->scope->varType->kind != Type_Named) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "expected expression");
  }

  struct Symbol *type = findSymbol(table->scope, &table->scope->varType->type_named.name);

  if (type == NULL) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'", 
        table->scope->varType->type_named.name.length, table->scope->varType->type_named.name.start);
  }

  if (type->kind != Symbol_Struct) {
    struct String str = getType(table->scope->varType, table->program->arena);
    errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
  }

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct Field *f1 = &expr->expr_struct.fields[i];
    struct Symbol *f2 = hashmap_get(type->symbol_struct.items, &f1->name);

    if (f2 == NULL) {
      errorLang(table->program->args->input_file, f1->line, f1->column, "cannot find name '%.*s'",
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
    errorLang(table->program->args->input_file, expr->line, expr->column, "'this' outside of struct method");
  }

  struct Symbol *member = hashmap_get(table->scope->currentStruct->symbol_struct.items, &expr->expr_this);
 
  if (member == NULL) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
        expr->expr_this.length, expr->expr_this.start, table->scope->currentStruct->name.length, table->scope->currentStruct->name.start);
  }

  return member;
}

struct Symbol *resolveExprIdentifier(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *symbol = findSymbol(table->scope, &expr->expr_identifier);

  if (symbol == NULL || expr->line < symbol->line || (expr->line == symbol->line && expr->column < symbol->column)) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'", expr->expr_identifier.length, expr->expr_identifier.start);
  }

  return symbol;
}
