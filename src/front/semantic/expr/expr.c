#include "../semantic.h"

struct Symbol *resolveExpr(struct SymbolTable *table, struct Expr *expr) {
  if (expr->kind == Expr_Assign) return resolveExprAssign(table, expr);
  else if (expr->kind == Expr_Logical) return resolveExprLogical(table, expr);
  else if (expr->kind == Expr_Compare) return resolveExprCompare(table, expr);
  else if (expr->kind == Expr_Binary) return resolveExprBinary(table, expr);
  else if (expr->kind == Expr_Cast) return resolveExprCast(table, expr);
  else if (expr->kind == Expr_Unary) return resolveExprUnary(table, expr);
  else if (expr->kind == Expr_Member) return resolveExprMember(table, expr);
  else if (expr->kind == Expr_Index) return resolveExprIndex(table, expr);
  else if (expr->kind == Expr_Call) return resolveExprCall(table, expr);
  else if (expr->kind == Expr_Callback) return resolveExprCallback(table, expr);
  else if (expr->kind == Expr_Struct) return resolveExprStruct(table, expr);
  else if (expr->kind == Expr_Array) return resolveExprArray(table, expr);
  else if (expr->kind == Expr_Identifier) return resolveExprIdentifier(table, expr);
  return NULL;
}
