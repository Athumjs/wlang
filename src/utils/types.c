#include <utils/types.h>
#include <utils/error.h>
#include <string.h>

struct String getType(struct Type *type, struct Arena *arena) {
  if (type->kind == Type_Auto) return (struct String) {
    .start = "auto",
    .length = 4
  };

  if (type->kind == Type_Primitive) {
    struct String str = {0};
    if (type->type_primitive.type == Primitive_Byte) {
      str.start = "byte";
      str.length = 4;
    }

    else if (type->type_primitive.type == Primitive_Ubyte) {
      str.start = "ubyte";
      str.length = 5;
    }

    else if (type->type_primitive.type == Primitive_Short) {
      str.start = "short";
      str.length = 5;
    }

    else if (type->type_primitive.type == Primitive_Ushort) {
      str.start = "ushort";
      str.length = 6;
    }

    else if (type->type_primitive.type == Primitive_Integer) {
      str.start = "int";
      str.length = 3;
    }

    else if (type->type_primitive.type == Primitive_Uinteger) {
      str.start = "uint";
      str.length = 4;
    }

    else if (type->type_primitive.type == Primitive_Long) {
      str.start = "long";
      str.length = 4;
    }

    else if (type->type_primitive.type == Primitive_Ulong) {
      str.start = "ulong";
      str.length = 5;
    }

    else if (type->type_primitive.type == Primitive_Float) {
      str.start = "float";
      str.length = 5;
    }

    else if (type->type_primitive.type == Primitive_Double) {
      str.start = "double";
      str.length = 6;
    }

    else if (type->type_primitive.type == Primitive_Char) {
      str.start = "char";
      str.length = 4;
    }

    else if (type->type_primitive.type == Primitive_Boolean) {
      str.start = "bool";
      str.length = 4;
    }

    else if (type->type_primitive.type == Primitive_Void) {
      str.start = "void";
      str.length = 4;
    }

    return str;
  }

  if (type->kind == Type_Named) return type->type_named.name;
  if (type->kind == Type_Array) {
    struct String str = getType(type->type_array.base, arena);
    char *start = arena_alloc(arena, str.length + 3);
    memcpy(start, str.start, str.length);
    start[str.length] = '[';
    start[str.length + 1] = ']';
    start[str.length + 2] = '\0';
    return (struct String) {
      .start = start,
      .length = str.length + 2
    };
  }

  if (type->kind == Type_Pointer) {
    struct String str = getType(type->type_pointer.base, arena);
    char *start = arena_alloc(arena, str.length + 2);
    memcpy(start, str.start, str.length);
    start[str.length] = '*';
    start[str.length + 1] = '\0';
    return (struct String) {
      .start = start,
      .length = str.length + 1
    };
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

  return (struct String) {
    .start = start,
    .length = len
  };
}

uint8_t cmpTT(struct SymbolTable *table, struct Type *t1, struct Type *t2) {
  if (t1->kind == Type_Auto || t2->kind == Type_Auto) return 1;

  if (t1->kind != t2->kind) return 0;

  if (t1->kind == Type_Primitive && t1->type_primitive.type == t2->type_primitive.type) return 1;

  if (t1->kind == Type_Named && t1->type_named.name.length == t2->type_named.name.length &&
        memcmp(t1->type_named.name.start, t2->type_named.name.start, t2->type_named.name.length) == 0) return 1;

  if (t1->kind == Type_Array) {
    return cmpTT(table, t1->type_array.base, t2->type_array.base);
  }

  if (t1->kind == Type_Pointer) {
    return cmpTT(table, t1->type_pointer.base, t2->type_pointer.base);
  }

  if (t1->kind == Type_Function) {
    if (!cmpTT(table, t1->type_function.retType, t2->type_function.retType)) return 0;
    if (t1->type_function.params_len != t2->type_function.params_len) return 0;
    for (int i = 0; i < t1->type_function.params_len; i++) {
      if (!cmpTT(table, t1->type_function.params[i], t2->type_function.params[i])) return 0;
    }
    return 1;
  }

  return 0;
}

uint8_t cmpTP(struct Type *t1, enum PrimitiveType t2) {
  if (t1->kind != Type_Primitive) return 0;
  return t1->type_primitive.type == t2;
}

void resolveType(struct SymbolTable *table, struct Type **type) {
  if (*type == NULL) {
    *type = arena_alloc(table->program->arena, sizeof(struct Type));
    (*type)->kind = Type_Auto;
    return;
  }
  
  if ((*type)->kind != Type_Named) {
    if ((*type)->kind == Type_Array) return resolveType(table, &(*type)->type_array.base);
    else if ((*type)->kind == Type_Pointer) return resolveType(table, &(*type)->type_pointer.base);
    else if ((*type)->kind == Type_Function) {
      resolveType(table, &(*type)->type_function.retType);
      for (int i = 0; i < (*type)->type_function.params_len; i++) {
        resolveType(table, &(*type)->type_function.params[i]);
      }
      return;
    }

    else return;
  }

#define X(pName, str) \
  else if ((*type)->type_named.name.length == sizeof(str) - 1 && memcmp((*type)->type_named.name.start, str, (*type)->type_named.name.length) == 0) { \
    *(*type) = (struct Type) { \
      .kind = Type_Primitive, \
      .type_primitive = { \
        .type = pName \
      } \
    }; \
    return; \
  }
  PRIMITIVE_TYPES
#undef X
}
