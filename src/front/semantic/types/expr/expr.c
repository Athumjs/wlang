#include "../../semantic.h"

struct Type *typeExpr(struct SymbolTable *table, struct Expr *expr) {
  if (expr->kind == Expr_Assign) return typeExprAssign(table, expr);
  else if (expr->kind == Expr_Logical) return typeExprLogical(table, expr);
  else if (expr->kind == Expr_Compare) return typeExprCompare(table, expr);
  else if (expr->kind == Expr_Binary) return typeExprBinary(table, expr);
  else if (expr->kind == Expr_Cast) return typeExprCast(table, expr);
  else if (expr->kind == Expr_Unary) return typeExprUnary(table, expr);
  else if (expr->kind == Expr_Member) return typeExprMember(table, expr);
  else if (expr->kind == Expr_Index) return typeExprIndex(table, expr);
  else if (expr->kind == Expr_Call) return typeExprCall(table, expr);
  else if (expr->kind == Expr_Callback) return typeExprCallback(table, expr);
  else if (expr->kind == Expr_Struct) return typeExprStruct(table, expr);
  else if (expr->kind == Expr_Array) return typeExprArray(table, expr);
  else if (expr->kind == Expr_Literal) return typeExprLiteral(table, expr);
  else if (expr->kind == Expr_This) return typeExprThis(table, expr);
  else if (expr->kind == Expr_Identifier) return typeExprIdentifier(table, expr);
  return NULL;
}
