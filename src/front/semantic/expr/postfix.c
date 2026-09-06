#include "../semantic.h"
#include <utils/hashmap.h>
#include <utils/error.h>

static inline uint8_t isCallable(struct Symbol *symbol) {
  return symbol != NULL && symbol->type->kind == Type_Function;
}

struct Symbol *resolveExprCall(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *callee = resolveExpr(table, expr->expr_call.callee);

  if (!isCallable(callee)) {
    errorLang(table->program->filename, expr->line, expr->column, "this expression is not callable");
  }

  for (int i = 0; i < expr->expr_call.args_len; i++) {
    resolveExpr(table, expr->expr_call.args[i]);
  }

  return callee;
}

static inline uint8_t isMemberAcessible(struct Symbol *symbol) {
  return symbol != NULL && (symbol->kind == Symbol_Variable || symbol->kind == Symbol_Enum);
}

struct Symbol *resolveExprMember(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *obj = resolveExpr(table, expr->expr_member.obj);

  if (!isMemberAcessible(obj)) {
    errorLang(table->program->filename, expr->line, expr->column, "member reference base type is not a structure");
  }

  struct Symbol *member = NULL;
  if (obj->kind == Symbol_Variable) {
    struct Symbol *symbol = findSymbol(table->scope, &obj->type->type_named.name);
    member = hashmap_get(symbol->symbol_struct.items, &expr->expr_member.member->expr_identifier.name);
  } else member = hashmap_get(obj->symbol_enum.items, &expr->expr_member.member->expr_identifier.name);

  if (member == NULL) {
    errorLang(table->program->filename, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
        expr->expr_member.member->expr_identifier.name.length, expr->expr_member.member->expr_identifier.name.start, obj->name.length, obj->name.start);
  }

  return member;
}

static inline uint8_t isArray_or_Pointer(struct Type *type) {
  return type->kind == Type_Array || type->kind == Type_Pointer;
}

struct Symbol *resolveExprIndex(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *base = resolveExpr(table, expr->expr_index.base);

  if (!isArray_or_Pointer(base->type)) {
    errorLang(table->program->filename, expr->line, expr->column, "subscripted value is not an array or pointer");
  }

  resolveExpr(table, expr->expr_index.index);
  return base;
}
