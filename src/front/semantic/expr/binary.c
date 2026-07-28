#include "../semantic.h"
#include <utils/error.h>

static inline uint8_t isAssignable(struct Symbol *symbol) {
  return symbol != NULL && symbol->kind == Symbol_Variable;
}

struct Symbol *resolveExprAssign(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *left = resolveExpr(table, expr->expr_binary.left);

  if (!isAssignable(left)) {
    errorLang(table->program->filename, expr->line, expr->column, "expression is not assignable");
  }

  if (left->symbol_variable.isConst) {
    errorLang(table->program->filename, expr->line, expr->column, "cannot assign to '%.*s' because it is a constant",
        left->name.length, left->name.start);
  }

  resolveExpr(table, expr->expr_binary.right);
  return NULL;
}

struct Symbol *resolveExprLogical(struct SymbolTable *table, struct Expr *expr) {
  resolveExpr(table, expr->expr_binary.left);
  resolveExpr(table, expr->expr_binary.right);
  return NULL;
}

struct Symbol *resolveExprCompare(struct SymbolTable *table, struct Expr *expr) {
  resolveExpr(table, expr->expr_binary.left);
  resolveExpr(table, expr->expr_binary.right);
  return NULL;
}

struct Symbol *resolveExprBinary(struct SymbolTable *table, struct Expr *expr) {
  resolveExpr(table, expr->expr_binary.left);
  resolveExpr(table, expr->expr_binary.right);
  return NULL;
}
