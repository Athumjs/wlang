#include <front/semantic.h>
#include <utils/hashmap.h>
#include <string.h>
#include <utils/error.h>

struct Value {
  struct Type *type;
  struct Symbol *symbol;
};

struct Value semanticExpr(struct SymbolTable *table, struct Expr *expr) {
  struct Value value = {
    .type = NULL,
    .symbol = NULL
  };

  if (expr->kind == Expr_Assign) {}

  else if (expr->kind == Expr_Logical) {}

  else if (expr->kind == Expr_Compare) {}

  else if (expr->kind == Expr_Binary) {}

  else if (expr->kind == Expr_Cast) {}

  else if (expr->kind == Expr_Unary) {}

  else if (expr->kind == Expr_Member) {
    struct Value obj = semanticExpr(table, expr->expr_member.obj);

    if (obj.symbol == NULL || obj.symbol->kind != Symbol_Struct) {
      struct String t = getType(obj.type, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "member reference base type '%.*s' is not a structure");
    }

    struct Value member = semanticExpr(table, expr->expr_member.member);

    if (member.symbol == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected identifier");
    }

    void *property = hashmap_get(obj.symbol->symbol_struct.items, &member.symbol->name);

    if (property == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
          member.symbol->name.length, member.symbol->name.start, obj.symbol->name.length, obj.symbol->name.start);
    }

    value.type = member.type;
    value.symbol = member.symbol;
    return value;
  }

  else if (expr->kind == Expr_Call) {
    struct Value callee = semanticExpr(table, expr->expr_call.callee);

    if (callee.symbol == NULL || callee.type->kind != Type_Function) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "this expression is not callable");
    }

    if (expr->expr_call.args_len != callee.type->type_function.params_len) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected %d arguments, but got %d",
          callee.symbol->symbol_function.params_len, expr->expr_call.args_len);
    }

    for (int i = 0; i < callee.type->type_function.params_len; i++) {
      struct Type *param = callee.type->type_function.params[i];
      struct Value arg = semanticExpr(table, expr->expr_call.args[i]);

      if (!cmpType(arg.type, param)) {
        struct String t1 = getType(arg.type, table->program->arena);
        struct String t2 = getType(param, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = callee.type->type_function.retType;
    value.symbol = callee.symbol;
    return value;
  }

  else if (expr->kind == Expr_Struct) {
    if (table->scope->expectType == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "cannot infer type of struct literal");
    }

    if (table->scope->expectType->kind != Type_User) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected expression");
    }

    struct Symbol *type = findSymbol(table->scope, &table->scope->expectType->type_user.name);

    if (type == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'", 
          table->scope->expectType->type_user.name.length, table->scope->expectType->type_user.name.start);
    }

    if (type->kind != Symbol_Struct) {
      struct String str = getType(table->scope->expectType, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
    }

    if (expr->expr_struct.properties_len > type->symbol_struct.length) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected 1-%d properties, but got %d",
          type->symbol_struct.length, expr->expr_struct.properties_len);
    }

    for (int i = 0; i < expr->expr_struct.properties_len; i++) {
      struct Property *p1 = &expr->expr_struct.properties[i];
      struct Label *p2 = hashmap_get(type->symbol_struct.items, &p1->name);

      if (!p2) {
        errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'",
            p1->name.length, p1->name.start);
      }

      struct Type *old = table->scope->expectType;
      table->scope->expectType = p2->type;
      struct Value v = semanticExpr(table, p1->expr);
      table->scope->expectType = old;

      if (!cmpType(p2->type, v.type)) {
        struct String t1 = getType(v.type, table->program->arena);
        struct String t2 = getType(p2->type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = table->scope->expectType;
    return value;
  }

  else if (expr->kind == Expr_Array) {
    value.type = arena_alloc(table->program->arena, sizeof(struct Type));
    value.type->kind = Type_Array;
    
    if (table->scope->expectType == NULL) {
      if (expr->expr_array.exprs_len == 0) value.type->type_array.base = NULL;
      else value.type->type_array.base = semanticExpr(table, expr->expr_array.exprs[0]).type; 
    } else {
      if (table->scope->expectType->kind != Type_Array) {
        struct String str = getType(table->scope->expectType, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
      }

      value.type->type_array.base = table->scope->expectType->type_array.base;
    }

    for (int i = 0; i < expr->expr_array.exprs_len; i++) {
      struct Type *old = table->scope->expectType;
      table->scope->expectType = value.type->type_array.base;
      struct Value exprValue = semanticExpr(table, expr->expr_array.exprs[i]);
      table->scope->expectType = old;

      if (!cmpType(exprValue.type, value.type->type_array.base)) {
        struct String s1 = getType(exprValue.type, table->program->arena);
        struct String s2 = getType(value.type->type_array.base, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            s1.length, s1.start, s2.length, s2.start);
      }
    }

    return value;
  }

  else if (expr->kind == Expr_Literal) {
    value.type = arena_alloc(table->program->arena, sizeof(struct Type));
    value.type->kind = Type_User;
    struct String str = {0};

    if (expr->expr_literal.kind == LITERAL_INTEGER) {
      str.start = "int";
      str.length = 3;
    }

    else if (expr->expr_literal.kind == LITERAL_UINTEGER) {
      str.start = "uint";
      str.length = 4;
    }

    else if (expr->expr_literal.kind == LITERAL_FLOAT) {
      str.start = "float";
      str.length = 5;
    }

    else if (expr->expr_literal.kind == LITERAL_DOUBLE) {
      str.start = "double";
      str.length = 6;
    }

    else if (expr->expr_literal.kind == LITERAL_CHAR) {
      str.start = "char";
      str.length = 4;
    }

    else if (expr->expr_literal.kind == LITERAL_STRING) {
      str.start = "string";
      str.length = 6;
    }

    else if (expr->expr_literal.kind == LITERAL_BOOLEAN) {
      str.start = "bool";
      str.length = 4;
    }

    value.type->type_user.name = str;
    return value;
  }

  struct Symbol *symbol = findSymbol(table->scope, &expr->expr_identifier);

  if (symbol == NULL) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "Cannot find name '%.*s'", expr->expr_identifier.length, expr->expr_identifier.start);
  }

  value.type = symbol->type;
  value.symbol = symbol;
  return value;
}

void semanticNode(struct SymbolTable *table, struct Node *node) {
  if (node->kind != Node_Expr) return;
  semanticExpr(table, node->node_expr);
}

void semantic(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Node *node = table->program->nodes[i];
    if (node->kind != Node_Function || (node->kind == Node_Public && node->node_public->kind != Node_Function)) continue;
    semanticNode(table, node);
  }

  for (int i = 0; i < table->program->length; i++) {
    struct Node *node = table->program->nodes[i];
    if (node->kind == Node_Function || (node->kind == Node_Public && node->node_public->kind == Node_Function)) continue;
    semanticNode(table, node);
  }
}
