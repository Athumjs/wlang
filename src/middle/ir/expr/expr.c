#include "expr.h"

struct IRExpr irExpr(struct Expr *expr, struct MIRContext *ctx) {
  struct IRExpr ir_expr = {0};

  if (expr->kind == Expr_Assign) ir_expr = irAssign(expr, ctx);
  else if (expr->kind == Expr_Logical) ir_expr = irLogical(expr, ctx);
  else if (expr->kind == Expr_Compare) ir_expr = irCompare(expr, ctx);
  else if (expr->kind == Expr_Binary) ir_expr = irBinary(expr, ctx);
  else if (expr->kind == Expr_Cast) ir_expr = irCast(expr, ctx);
  else if (expr->kind == Expr_Unary) ir_expr = irUnary(expr, ctx);
  else if (expr->kind == Expr_Member) ir_expr = irMember(expr, ctx);
  else if (expr->kind == Expr_Index) ir_expr = irIndex(expr, ctx);
  else if (expr->kind == Expr_Call) ir_expr = irCall(expr, ctx);
  else if (expr->kind == Expr_Struct) ir_expr = irStructExpr(expr, ctx);
  else if (expr->kind == Expr_Array) ir_expr = irArray(expr, ctx);
  else if (expr->kind == Expr_Identifier) ir_expr = irId(expr, ctx);
  else if (expr->kind == Expr_This) ir_expr = irThis(expr, ctx);
  else if (expr->kind == Expr_Literal) ir_expr = irLiteral(expr, ctx);

  if (ir_expr.block == NULL) ir_expr.block = ctx->block;
  if (expr->kind != Expr_Unary || expr->expr_unary.op != TOKEN_BIT_AND) ir_expr.operand.op_type = resolveTypeIR(expr->type, ctx->arena);
  return ir_expr;
}
