#pragma once

#include <front/semantic.h>
#include <utils/arena.h>
#include <utils/literal.h>

struct SymbolTable;

#define PRIMITIVE_TYPES \
  X(Primitive_Byte, "byte") \
  X(Primitive_Ubyte, "ubyte") \
  X(Primitive_Short, "short") \
  X(Primitive_Ushort, "ushort") \
  X(Primitive_Integer, "int") \
  X(Primitive_Uinteger, "uint") \
  X(Primitive_Long, "long") \
  X(Primitive_Ulong, "ulong") \
  X(Primitive_Float, "float") \
  X(Primitive_Double, "double") \
  X(Primitive_Char, "char") \
  X(Primitive_Boolean, "bool") \
  X(Primitive_Void, "void")

enum TypeKind {
  Type_Auto,
  Type_Primitive,
  Type_Named,
  Type_Array,
  Type_Pointer,
  Type_Function
};

enum PrimitiveType {
#define X(name, str) name,
  PRIMITIVE_TYPES
#undef X
};

struct Type {
  enum TypeKind kind;

  union {
    struct {
      enum PrimitiveType type;
    } type_primitive;

    struct {
      struct String name;
    } type_named;

    struct {
      struct Type *base;
      struct Expr *expr;
      size_t length;
    } type_array;
    
    struct {
      struct Type *base;
    } type_pointer;

    struct {
      struct Type *retType;
      struct Type **params;
      size_t params_len;
      size_t params_cap;
    } type_function;
  };
};

struct String getType(struct Type *type, struct Arena *arena);
uint8_t arrayIsConstant(struct SymbolTable *table, struct Expr *expr, size_t *value);
uint8_t cmpType(struct SymbolTable *table, struct Type *t1, struct Type *t2);
void resolveType(struct SymbolTable *table, struct Type **type);
