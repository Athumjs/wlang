#include <utils/numeric.h>

uint8_t isInteger(struct Type *type) {
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

uint8_t isFloating(struct Type *type) {
  if (type->kind != Type_Primitive) return 0;

  switch (type->type_primitive.type) {
    case Primitive_Float:
    case Primitive_Double:
      return 1;

    default:
      return 0;
  }
}

uint8_t isSignedInteger(struct Type *type) {
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

uint8_t isUnsignedInteger(struct Type *type) {
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
