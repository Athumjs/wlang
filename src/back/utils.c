#include "back.h"

size_t sizeOf(struct IRType *type) {
  if (type->kind == IRTYPEK_ARRAY)
    return type->array.size * sizeOf(type->array.type);

  if (type->kind == IRTYPEK_OTHER) {
    switch (type->other) {
      case IRTYPE_i1:
      case IRTYPE_i8:
        return 1;
      case IRTYPE_i16:
        return 2;
      case IRTYPE_f32:
      case IRTYPE_i32:
        return 4;
      case IRTYPE_f64:
      case IRTYPE_i64:
      case IRTYPE_ptr:
        return 8;
      default:
        return 0;
    }
  }

  return 0;
}

size_t alignOf(struct IRType *type) {
  if (type->kind == IRTYPEK_ARRAY)
    return alignOf(type->array.type);
  return sizeOf(type);
}
