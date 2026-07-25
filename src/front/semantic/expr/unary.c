#include "../semantic.h" 
#include <utils/error.h>

struct Symbol *resolveExprCast(struct SymbolTable *table, struct Expr *expr) {
  resolveExpr(table, expr->expr_cast.value);
  return NULL;
}

struct Symbol *resolveExprUnary(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *arg = resolveExpr(table, expr->expr_unary.arg);

  if (expr->expr_unary.op == TOKEN_INCREMENT || expr->expr_unary.op == TOKEN_DECREMENT) {
    if (arg == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expression is not assignable");
    }
  }

  return NULL;
}
