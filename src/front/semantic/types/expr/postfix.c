#include "../../semantic.h"
#include "expr.h"
#include <utils/hashmap.h>
#include <utils/error.h>

struct Type *typeExprCall(struct SymbolTable *table, struct Expr *expr) {
  struct Type *callee = typeExpr(table, expr->expr_call.callee);

  if (callee->kind != Type_Function) {
    errorLang(table->program->filename, expr->line, expr->column, "this expression is not callable");
  }

  if (callee->type_function.params_len != expr->expr_call.args_len) {
      errorLang(table->program->filename, expr->line, expr->column, "expected %d arguments, but got %d",
          callee->type_function.params_len, expr->expr_call.args_len);
  }

  for (int i = 0; i < callee->type_function.params_len; i++) {
    struct Type *param = callee->type_function.params[i];
    struct Type *arg = typeExpr(table, expr->expr_call.args[i]);

    if (!cmpTT(table, arg, param) && !canImplicitConvert(table, arg, param)) {
      struct String s1 = getType(arg, table->program->arena);
      struct String s2 = getType(param, table->program->arena);
      errorLang(table->program->filename, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
          s1.length, s1.start, s2.length, s2.start);
    }

    expr->expr_call.args[i]->type = param;
  }

  expr->type = callee->type_function.retType;
  return callee->type_function.retType;
}

struct Type *typeExprMember(struct SymbolTable *table, struct Expr *expr) {
  typeExpr(table, expr->expr_member.obj);
  struct Symbol *obj = resolveExpr(table, expr->expr_member.obj);

  struct Symbol *member = NULL;
  if (obj->kind == Symbol_Variable) {
    struct Symbol *symbol = findSymbol(table->scope, &obj->type->type_named.name);
    member = hashmap_get(symbol->symbol_struct.items, &expr->expr_member.member->expr_identifier);
  } else member = hashmap_get(obj->symbol_enum.items, &expr->expr_member.member->expr_identifier);

  expr->type = member->type;
  return member->type;
}

struct Type *typeExprIndex(struct SymbolTable *table, struct Expr *expr) {
  struct Type *base = typeExpr(table, expr->expr_index.base);
  struct Type *index = typeExpr(table, expr->expr_index.index);

  if (!isInteger(index)) {
    errorLang(table->program->filename, expr->line, expr->column, "array subscript is not a integer");
  }

  if (base->kind == Type_Array) {
    expr->type = base->type_array.base;
  } else {
    expr->type = base->type_pointer.base;
  }

  return expr->type;
}
