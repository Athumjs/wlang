#include "../../semantic.h"
#include "expr.h"
#include <utils/error.h>

struct Type *typeExprAssign(struct SymbolTable *table, struct Expr *expr) {
  struct Type *left = typeExpr(table, expr->expr_binary.left);
  struct Type *right = typeExpr(table, expr->expr_binary.right);

  if (left->kind == Type_Auto) {
    struct Symbol *lvalue = findSymbol(table->scope, &expr->expr_binary.left->expr_identifier);
    lvalue->type = right;
    left = right;
  }

  if (!canImplicitConvert(table, right, left)) {
    struct String s1 = getType(right, table->program->arena);
    struct String s2 = getType(left, table->program->arena);
    errorLang(table->program->filename, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
        s1.length, s1.start, s2.length, s2.start);
  }

  expr->expr_binary.right->type = left;
  expr->type = left;
  return left;
}

struct Type *typeExprLogical(struct SymbolTable *table, struct Expr *expr) {
  struct Type *left = typeExpr(table, expr->expr_binary.left);
  struct Type *right = typeExpr(table, expr->expr_binary.right);

  if (!isBoolean(left) || !isBoolean(right)) {
    struct String s1 = getType(left, table->program->arena);
    struct String s2 = getType(right, table->program->arena);
    errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
        tk_names[expr->expr_binary.op], s1.length, s1.start, s2.length, s2.start);
  }

  expr->type = typePrimitive(table, Primitive_Boolean);
  return expr->type;
}

static inline uint8_t canEquality(struct SymbolTable *table, struct Type *t1, struct Type *t2) {
  if (cmpTP(t1, Primitive_Char) || cmpTP(t2, Primitive_Char) || cmpTP(t1, Primitive_Boolean) || cmpTP(t2, Primitive_Boolean) ||
      t1->kind == Type_Named || t2->kind == Type_Named)
    return cmpTT(table, t1, t2);

  return canImplicitConvert(table, t1, t2) || canImplicitConvert(table, t2, t1);
}

struct Type *typeExprCompare(struct SymbolTable *table, struct Expr *expr) {
  struct Type *left = typeExpr(table, expr->expr_binary.left);
  struct Type *right = typeExpr(table, expr->expr_binary.right);

  if (expr->expr_binary.op == TOKEN_GT || expr->expr_binary.op == TOKEN_GE ||
      expr->expr_binary.op == TOKEN_LT || expr->expr_binary.op == TOKEN_LE) {
    if (!canImplicitConvert(table, left, right) && !canImplicitConvert(table, right, left)) {
      struct String s1 = getType(left, table->program->arena);
      struct String s2 = getType(right, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
          tk_names[expr->expr_binary.op], s1.length, s1.start, s2.length, s2.start);
    }

    if (canImplicitConvert(table, left, right)) {
      expr->expr_binary.left->type = right;
    } else expr->expr_binary.right->type = left;
  }

  else if (expr->expr_binary.op == TOKEN_EQ || expr->expr_binary.op == TOKEN_NE) {
    if (!canEquality(table, left, right)) {
      struct String s1 = getType(left, table->program->arena);
      struct String s2 = getType(right, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
          tk_names[expr->expr_binary.op], s1.length, s1.start, s2.length, s2.start);
    }
  }

  expr->type = typePrimitive(table, Primitive_Boolean);
  return expr->type;
}

struct Type *typeExprBinary(struct SymbolTable *table, struct Expr *expr) {
  struct Type *left = typeExpr(table, expr->expr_binary.left);
  struct Type *right = typeExpr(table, expr->expr_binary.right);

  if (!canImplicitConvert(table, left, right) && !canImplicitConvert(table, right, left)) {
      struct String s1 = getType(left, table->program->arena);
      struct String s2 = getType(right, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
          tk_names[expr->expr_binary.op], s1.length, s1.start, s2.length, s2.start);
  }

  if (!cmpTT(table, left, right)) {
    if (canImplicitConvert(table, left, right)) {
      expr->expr_binary.left->type = right;
      expr->type = right;
      return right;
    } else expr->expr_binary.right->type = left;
  }

  expr->type = left;
  return left;
}
