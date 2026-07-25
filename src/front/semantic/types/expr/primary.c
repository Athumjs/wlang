#include "../../semantic.h"
#include "expr.h"
#include <utils/hashmap.h>
#include <utils/error.h>

struct Type *typeExprStruct(struct SymbolTable *table, struct Expr *expr) { 
  struct Symbol *type = findSymbol(table->scope, &table->scope->varType->type_named.name);

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct Field *f1 = &expr->expr_struct.fields[i];
    struct Symbol *f2 = hashmap_get(type->symbol_struct.items, &f1->name);

    if (f2 == NULL) {
      errorLang(table->program->args->input_file, f1->line, f1->column, "cannot find name '%.*s'",
          f1->name.length, f1->name.start);
    }

    struct Type *value = typeExpr(table, f1->expr);

    if (!cmpTT(table, value, f2->type)) {
      struct String t1 = getType(value, table->program->arena);
      struct String t2 = getType(f2->type, table->program->arena);
      errorLang(table->program->args->input_file, f1->line, f1->column, "type '%.*s' is not assignable to type '%.*s'",
          t1.length, t1.start, t2.length, t2.start);
    }
  }

  expr->type = table->scope->varType;
  return expr->type;
}

struct Type *typeExprArray(struct SymbolTable *table, struct Expr *expr) {
  struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
  type->kind = Type_Array;

  if (table->scope->varType == Type_Auto) {
    type->type_array.base = typeExpr(table, expr->expr_array.exprs[0]);
  } else {
    if (table->scope->varType->kind != Type_Array) {
      struct String str = getType(table->scope->varType, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
    }

    type->type_array.base = table->scope->varType->type_array.base;
  }

  for (int i = 0; i < expr->expr_array.exprs_len; i++) {
    struct Type *value = typeExpr(table, expr->expr_array.exprs[i]);

    if (!cmpTT(table, value, type->type_array.base)) {
      struct String s1 = getType(value, table->program->arena);
      struct String s2 = getType(type->type_array.base, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
          s1.length, s1.start, s2.length, s2.start);
    }
  }

  expr->type = type;
  return type;
}

struct Type *typeExprLiteral(struct SymbolTable *table, struct Expr *expr) {
  if (expr->expr_literal.kind == LITERAL_INTEGER) {
    expr->type = typePrimitive(table, Primitive_Integer);
    return expr->type;
  }

  else if (expr->expr_literal.kind == LITERAL_UINTEGER) {
    expr->type = typePrimitive(table, Primitive_Uinteger);
    return expr->type;
  }

  else if (expr->expr_literal.kind == LITERAL_FLOAT) {
    expr->type = typePrimitive(table, Primitive_Float);
    return expr->type;
  }

  else if (expr->expr_literal.kind == LITERAL_DOUBLE) {
    expr->type = typePrimitive(table, Primitive_Double);
    return expr->type;
  }

  else if (expr->expr_literal.kind == LITERAL_CHAR) {
    expr->type = typePrimitive(table, Primitive_Char);
    return expr->type;
  }

  else if (expr->expr_literal.kind == LITERAL_STRING) {
    struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
    type->kind = Type_Pointer;
    type->type_pointer.base = typePrimitive(table, Primitive_Char);
    expr->type = type;
    return expr->type;
  }

  else {
    expr->type = typePrimitive(table, Primitive_Boolean);
    return expr->type;
  }
}

struct Type *typeExprThis(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *member = hashmap_get(table->scope->currentStruct->symbol_struct.items, &expr->expr_this);
  expr->type = member->type;
  return member->type;
}

struct Type *typeExprIdentifier(struct SymbolTable *table, struct Expr *expr) {
  struct Symbol *symbol = findSymbol(table->scope, &expr->expr_identifier);
  expr->type = symbol->type;
  return symbol->type;
}
