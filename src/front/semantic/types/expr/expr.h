#pragma once

#include <utils/numeric.h>

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

static inline uint8_t isBoolean(struct Type *type) {
  return type->kind == Type_Primitive && type->type_primitive.type == Primitive_Boolean;
}

static inline uint8_t isPointer(struct Type *type) {
  return type->kind == Type_Pointer;
}

static inline uint8_t isArray(struct Type *type) {
  return type->kind == Type_Array;
}

static inline uint8_t isFunction(struct Type *type) {
  return type->kind == Type_Function;
}

static inline uint8_t isChar(struct Type *type) {
  return type->kind == Type_Primitive && type->type_primitive.type == Primitive_Char;
}

static inline uint8_t canImplicitConvert(struct SymbolTable *table, struct Type *t1, struct Type *t2) {
  if (cmpTT(table, t1, t2)) return 1;
  if (isFunction(t1) && isFunction(t2))
    return canImplicitConvert(table, t1->type_function.retType, t2->type_function.retType);

  if (isInteger(t1) && isBoolean(t2))
    return 1;

  if (isPointer(t1) && isPointer(t2))
    return canImplicitConvert(table, t1->type_pointer.base, t2->type_pointer.base);

  if (isArray(t1) && isPointer(t2))
    return canImplicitConvert(table, t1->type_array.base, t2->type_pointer.base);

  if (isPointer(t1) && isArray(t2))
    return canImplicitConvert(table, t1->type_pointer.base, t2->type_array.base);

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

static inline uint8_t isByte(int64_t num) {
  return num >= -128 && num <= 127;
}

static inline uint8_t isShort(int64_t num) {
  return num >= -32768 && num <= 32767;
}

static inline uint8_t isInt(int64_t num) {
  return num >= -2147483648 && num <= 2147483647;
}

static inline uint8_t isLong(int64_t num) {
  return num >= -9223372036854775808 && num <= 9223372036854775807;
}

static inline uint8_t isUbyte(uint64_t num) {
  return num >= 0 && num <= 255u;
}

static inline uint8_t isUshort(uint64_t num) {
  return num >= 0 && num <= 65535u;
}

static inline uint8_t isUint(uint64_t num) {
  return num >= 0 && num <= 4294967295u;
}

static inline uint8_t isUlong(uint64_t num) {
  return num >= 0 && num <= 18446744073709551615u;
}
