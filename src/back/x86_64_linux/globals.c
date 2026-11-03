#include "x86_64_linux.h"

static void resolveType_code(struct IRType *type, FILE *file) {
  if (type->kind == IRTYPEK_ARRAY)
    return resolveType_code(type->array.type, file);

  if (type->kind != IRTYPEK_OTHER) return;
  if (type->other == IRTYPE_i1 || type->other == IRTYPE_i8)
    fprintf(file, ".byte");
  if (type->other == IRTYPE_i16)
    fprintf(file, ".word");
  if (type->other == IRTYPE_i32 || type->other == IRTYPE_f32)
    fprintf(file, ".long");
  if (type->other == IRTYPE_i64 || type->other == IRTYPE_f64 || type->other == IRTYPE_ptr)
    fprintf(file, ".quad");
}

static void resolveValue_code(struct IRValue *value, FILE *file) {
  if (value->kind == Value_Int)
    fprintf(file, "%ld", value->i);
  else if (value->kind == Value_UInt || value->kind == Value_Char)
    fprintf(file, "%lu", value->u);
  else if (value->kind == Value_Float)
    fprintf(file, "%f", value->f);
  else if (value->kind == Value_Double)
    fprintf(file, "%lf", value->d);
  else if (value->kind == Value_String)
    fprintf(file, "\"%.*s\"", value->s->length, value->s->start);

  else if (value->kind == Value_Array) {
    for (int i = 0; i < value->a.elems_len; i++) {
      if (i > 0) fprintf(file, ", ");
      resolveValue_code(&value->a.elems[i], file);
    }
  }
}

void x86_64_linux_globals(struct IRGlobal *globals, size_t globals_len, FILE *file) {
  if (globals_len > 0) fprintf(file, ".section .data\n");
  uint8_t bss = 0;

  for (int i = 0; i < globals_len; i++) {
    if (globals[i].value.constant.kind == Value_Int && globals[i].value.constant.i == 0) {
      bss = 1;
      continue;
    }

    fprintf(file, "  g%ld: ", globals[i].vl);
    if (globals[i].value.constant.kind == Value_String)
      fprintf(file, ".asciz");
    else resolveType_code(&globals[i].type, file);
    fprintf(file, " ");
    resolveValue_code(&globals[i].value.constant, file);
    fprintf(file, "\n");
  }

  if (bss) {
    fprintf(file, ".section .bss");

    for (int i = 0; i < globals_len; i++) {
      if (globals[i].value.constant.kind != Value_Int || globals[i].value.constant.i != 0) continue;
      fprintf(file, "  g%ld: .space %zu\n", globals[i].vl, sizeOf(&globals[i].type));
    }
  }
}
