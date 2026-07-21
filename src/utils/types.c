#include <utils/types.h>
#include <front/parser.h>
#include <string.h>

struct String getType(struct Type *type, struct Arena *arena) {
  if (type->kind == Type_User) return type->type_user.name;
  if (type->kind == Type_Array) {
    struct String str = getType(type->type_array.base, arena);
    int i = 0;
    while (*str.start != ']') {
      i++;
      str.start++;
    }
    str.start -= i;
    str.length += i - 2;
    return str;
  }

  int length = 6;
  for (int i = 0; i < type->type_function.params_len; i++) {
    if (i > 0) length += 2;
    length += getType(type->type_function.params[i], arena).length;
  }
  length += getType(type->type_function.retType, arena).length;
  char *start = arena_alloc(arena, length + 1);
  int len = 0;
  start[len++] = '(';

  for (int i = 0; i < type->type_function.params_len; i++) {
    if (i > 0) {
      start[len++] = ',';
      start[len++] = ' ';
    }
    struct String t = getType(type->type_function.params[i], arena);
    memcpy(start + len, t.start, t.length);
    len += t.length;
  }
  start[len++] = ')';
  start[len++] = ' ';
  start[len++] = '=';
  start[len++] = '>';
  start[len++] = ' ';
  struct String t = getType(type->type_function.retType, arena);
  memcpy(start + len, t.start, t.length);
  len += t.length;
  start[len] = '\0';

  struct String str = {
    .start = start,
    .length = len
  };
  return str;
}

uint8_t cmpExpr(struct Expr *e1, struct Expr *e2) {
  if (e1->kind != e2->kind) return 0;

  if (e1->kind == Expr_Assign) {
    return cmpExpr(e1->expr_binary.left, e2->expr_binary.left) && e1->expr_binary.op == e2->expr_binary.op &&
      cmpExpr(e1->expr_binary.right, e2->expr_binary.right);
  }

  if (e1->kind == Expr_Logical) {
    return cmpExpr(e1->expr_binary.left, e2->expr_binary.left) && e1->expr_binary.op == e2->expr_binary.op &&
      cmpExpr(e1->expr_binary.right, e2->expr_binary.right);
  }

  if (e1->kind == Expr_Compare) {
    return cmpExpr(e1->expr_binary.left, e2->expr_binary.left) && e1->expr_binary.op == e2->expr_binary.op &&
      cmpExpr(e1->expr_binary.right, e2->expr_binary.right);
  }

  if (e1->kind == Expr_Binary) {
    return cmpExpr(e1->expr_binary.left, e2->expr_binary.left) && e1->expr_binary.op == e2->expr_binary.op &&
      cmpExpr(e1->expr_binary.right, e2->expr_binary.right);
  }

  if (e1->kind == Expr_Cast) {
    return cmpType(e1->expr_cast.type, e2->expr_cast.type) && cmpExpr(e1->expr_cast.value, e2->expr_cast.value);
  }

  if (e1->kind == Expr_Unary) {
    return cmpExpr(e1->expr_unary.arg, e2->expr_unary.arg) && e1->expr_unary.op == e2->expr_unary.op &&
      e1->expr_unary.prefix == e2->expr_unary.prefix;
  }

  if (e1->kind == Expr_Member) {
    return cmpExpr(e1->expr_member.obj, e2->expr_member.obj) && cmpExpr(e1->expr_member.member, e2->expr_member.member);
  }

  if (e1->kind == Expr_Call) {
    if (!cmpExpr(e1->expr_call.callee, e2->expr_call.callee) || e1->expr_call.args_len != e2->expr_call.args_len) return 0;
    for (int i = 0; i < e1->expr_call.args_len; i++) {
      if (!cmpExpr(e1->expr_call.args[i], e2->expr_call.args[i])) return 0;
    }
    return 1;
  }

  if (e1->kind == Expr_Struct) {
    if (e1->expr_struct.properties_len != e2->expr_struct.properties_len) return 0;
    for (int i = 0; i < e1->expr_struct.properties_len; i++) {
      struct Property *p1 = &e1->expr_struct.properties[i];
      struct Property *p2 = &e2->expr_struct.properties[i];

      if (!cmpExpr(p1->expr, p2->expr)) return 0;
      if (p1->name.length != p2->name.length || memcmp(p1->name.start, p2->name.start, p2->name.length) != 0) return 0;
    }
    return 1;
  }

  if (e1->kind == Expr_Array) {
    if (e1->expr_array.exprs_len != e2->expr_array.exprs_len) return 0;
    for (int i = 0; i < e1->expr_array.exprs_len; i++) {
      if (!cmpExpr(e1->expr_array.exprs[i], e2->expr_array.exprs[i])) return 0;
    }
    return 1;
  }

  if (e1->kind == Expr_Literal) {
    if (e1->expr_literal.kind != e2->expr_literal.kind) return 0;

    if (e1->expr_literal.kind == LITERAL_INTEGER) return e1->expr_literal.literal.numInt == e2->expr_literal.literal.numInt;
    else if (e1->expr_literal.kind == LITERAL_UINTEGER) return e1->expr_literal.literal.numUint == e2->expr_literal.literal.numUint;
    else if (e1->expr_literal.kind == LITERAL_FLOAT) return e1->expr_literal.literal.numFloat == e2->expr_literal.literal.numFloat;
    else if (e1->expr_literal.kind == LITERAL_DOUBLE) return e1->expr_literal.literal.numDouble == e2->expr_literal.literal.numDouble;
    else return e1->expr_literal.literal.string.length == e2->expr_literal.literal.string.length &&
      memcmp(e1->expr_literal.literal.string.start, e2->expr_literal.literal.string.start, e2->expr_literal.literal.string.length) == 0;

    return 1;
  }

  if (e1->kind == Expr_Identifier) {
    return e1->expr_identifier.length == e2->expr_identifier.length &&
      memcmp(e1->expr_identifier.start, e2->expr_identifier.start, e2->expr_identifier.length) == 0;
  }

  return 0;
}

uint8_t cmpType(struct Type *t1, struct Type *t2) {
  if (t1->kind != t2->kind) return 0;
  if (t1->kind == Type_User && t1->type_user.name.length == t2->type_user.name.length &&
        memcmp(t1->type_user.name.start, t2->type_user.name.start, t2->type_user.name.length) == 0) return 1;

  if (t1->kind == Type_Array) {
    return cmpType(t1->type_array.base, t2->type_array.base) && cmpExpr(t1->type_array.expr, t2->type_array.expr);
  }

  if (t1->kind == Type_Function) {
    if (!cmpType(t1->type_function.retType, t2->type_function.retType)) return 0;
    if (t1->type_function.params_len != t2->type_function.params_len) return 0;
    for (int i = 0; i < t1->type_function.params_len; i++) {
      if (!cmpType(t1->type_function.params[i], t2->type_function.params[i])) return 0;
    }
    return 1;
  }

  return 0;
}
