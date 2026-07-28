#include "../../semantic.h"
#include "expr.h"
#include <utils/error.h>

static inline uint8_t canExplicitConvert(struct Type *t1, struct Type *t2) {
  if (isPointer(t1) && isPointer(t2)) return 1;
  if (isNumeric(t1) && isNumeric(t2)) return 1;
  if (isInteger(t1) && isChar(t2)) return 1;
  if (isInteger(t2) && isChar(t1)) return 1;

  return 0;
}

struct Type *typeExprCast(struct SymbolTable *table, struct Expr *expr) {
  resolveType(table, &expr->expr_cast.type);
  struct Type *value = typeExpr(table, expr->expr_cast.value);

  if (!canExplicitConvert(value, expr->expr_cast.type)) {
    struct String s1 = getType(value, table->program->arena);
    struct String s2 = getType(expr->expr_cast.type, table->program->arena);
    errorLang(table->program->filename, expr->line, expr->column, "cannot cast type '%.*s' to type '%.*s'",
        s1.length, s1.start, s2.length, s2.start);
  }

  expr->expr_cast.value->type = expr->expr_cast.type;
  expr->type = expr->expr_cast.type;
  return expr->expr_cast.type;
}

struct Type *typeExprUnary(struct SymbolTable *table, struct Expr *expr) {
  struct Type *arg = typeExpr(table, expr->expr_unary.arg);

  if (expr->expr_unary.op == TOKEN_NOT) {
    if (!isBoolean(arg)) {
      struct String s1 = getType(arg, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
          tk_names[expr->expr_unary.op], s1.length, s1.start);
    }
  }

  else if (expr->expr_unary.op == TOKEN_BIT_NOT) {
    if (!isInteger(arg)) {
      struct String s1 = getType(arg, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
          tk_names[expr->expr_unary.op], s1.length, s1.start);
    }
  }

  else if (expr->expr_unary.op == TOKEN_MINUS) {
    if (!isNumeric(arg) || isUnsignedInteger(arg)) {
      struct String s1 = getType(arg, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
          tk_names[expr->expr_unary.op], s1.length, s1.start);
    }
  }

  else if (expr->expr_unary.op == TOKEN_ASTERISK) {
    if (!isPointer(arg)) {
      struct String s1 = getType(arg, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
          tk_names[expr->expr_unary.op], s1.length, s1.start);
    }

    expr->type = arg->type_pointer.base;
    return expr->type;
  }

  else if (expr->expr_unary.op == TOKEN_BIT_AND) {
    expr->type = arena_alloc(table->program->arena, sizeof(struct Type));
    expr->type->kind = Type_Pointer;
    expr->type->type_pointer.base = arg;
    return expr->type;
  }

  else if (expr->expr_unary.op == TOKEN_INCREMENT || expr->expr_unary.op == TOKEN_DECREMENT) {
    if (!isNumeric(arg) || !isPointer(arg)) {
      struct String s1 = getType(arg, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
          tk_names[expr->expr_unary.op], s1.length, s1.start);
    }
  }

  expr->type = arg;
  return arg;
}
