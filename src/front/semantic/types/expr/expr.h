#pragma once

#include <utils/types.h>

static inline uint8_t rankType(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Byte:
    case Primitive_Ubyte:
      return 1;

    case Primitive_Short:
    case Primitive_Ushort:
      return 2;

    case Primitive_Integer:
    case Primitive_Uinteger:
      return 3;

    case Primitive_Long:
    case Primitive_Ulong:
      return 4;

    case Primitive_Float:
      return 5;

    case Primitive_Double:
      return 6;

    default:
      return 0;
  }
}

static inline uint8_t isNumeric(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Ubyte:
    case Primitive_Ushort:
    case Primitive_Uinteger:
    case Primitive_Ulong:
    case Primitive_Byte:
    case Primitive_Short:
    case Primitive_Integer:
    case Primitive_Long:
    case Primitive_Float:
    case Primitive_Double:
      return 1;

    default:
      return 0;
  }
}

static inline uint8_t isInteger(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Ubyte:
    case Primitive_Ushort:
    case Primitive_Uinteger:
    case Primitive_Ulong:
    case Primitive_Byte:
    case Primitive_Short:
    case Primitive_Integer:
    case Primitive_Long:
      return 1;

    default:
      return 0;
  }
}

static inline uint8_t isFloating(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Float:
    case Primitive_Double:
      return 1;

    default:
      return 0;
  }
}

static inline uint8_t isSignedInteger(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Byte:
    case Primitive_Short:
    case Primitive_Integer:
    case Primitive_Long:
      return 1;

    default:
      return 0;
  }
}

static inline uint8_t isUnsignedInteger(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Ubyte:
    case Primitive_Ushort:
    case Primitive_Uinteger:
    case Primitive_Ulong:
      return 1;

    default:
      return 0;
  }
}

static inline uint8_t isBoolean(struct Type *type) {
  return type->kind == Type_Primitive && type->type_primitive.type == Primitive_Boolean;
}

static inline uint8_t isPointer(struct Type *type) {
  return type->kind == Type_Pointer;
}

static inline uint8_t isChar(struct Type *type) {
  return type->kind == Type_Primitive && type->type_primitive.type == Primitive_Char;
}

static inline uint8_t canImplicitConvert(struct SymbolTable *table, struct Type *t1, struct Type *t2) {
  if (cmpTT(table, t1, t2)) return 1;
  if (isSignedInteger(t1) && isSignedInteger(t2))
    return rankType(t1) <= rankType(t2);

  if (isUnsignedInteger(t1) && isUnsignedInteger(t2))
    return rankType(t1) <= rankType(t2);

  if (isFloating(t1) && isFloating(t2))
    return rankType(t1) <= rankType(t2);

  return 0;
}

static inline struct Type *typePrimitive(struct SymbolTable *table, enum PrimitiveType t) {
  struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
  type->kind = Type_Primitive;
  type->type_primitive.type = t;
  return type;
}
