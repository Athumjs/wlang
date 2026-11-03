#include "backs.h"
#include <stdlib.h>
#include <string.h>

FILE *create_as(const char *folder_name, const char *filename, char **name, int *name_len) {
  char *lastslash = strrchr(filename, '/');
  lastslash++;
  char *lastdot = strrchr(filename, '.');
  *name_len = lastdot - lastslash;
  int len = strlen(folder_name) + *name_len + 4;
  char *path = malloc(len);
  snprintf(path, len, "%s/%.*s.s", folder_name, *name_len, lastslash);
  FILE *file = fopen(path, "w");
  *name = lastslash;
  free(path);
  return file;
}

char *run_assembler(const char *folder_name, const char *name, int name_len, FILE *file) {
  int obj_len = strlen(folder_name) + name_len + 4;
  char *obj_name = malloc(obj_len);
  snprintf(obj_name, obj_len, "%s/%.*s.o", folder_name, name_len, name);
  int len = strlen("as -o ") + obj_len + strlen(folder_name) + name_len + 5;
  char *command = malloc(len);
  snprintf(command, len, "as -o %s %s/%.*s.s", obj_name, folder_name, name_len, name);
  system(command);
  free(command);
  return obj_name;
}

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
