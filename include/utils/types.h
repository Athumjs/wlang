#pragma once

#include <utils/literal.h>

enum TypeKind {
  Type_User,
  Type_Array,
  Type_Function
};

struct Type {
  enum TypeKind kind;

  union {
    struct {
      struct String name;
    } type_user;

    struct {
      struct Type *base;
      struct Expr *expr;
    } type_array;

    struct {
      struct Type *retType;
      struct Type *params;
      size_t params_len;
      size_t params_cap;
    } type_function;
  };
};
