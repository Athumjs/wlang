#include "back.h"

void resolveType_code(struct IRType *type, FILE *file) {
  if (type->kind == IRTYPEK_ARRAY)
    return resolveType_code(type->array.type, file);

  if (type->kind != IRTYPEK_OTHER) return;
  if (type->other == IRTYPE_i1 || type->other == IRTYPE_i8)
    fprintf(file, "db");
  if (type->other == IRTYPE_i16)
    fprintf(file, "dw");
  if (type->other == IRTYPE_i32 || type->other == IRTYPE_f32)
    fprintf(file, "dd");
  if (type->other == IRTYPE_i64 || type->other == IRTYPE_f64 || type->other == IRTYPE_ptr)
    fprintf(file, "dq");
}

void resolveGlobals_code(struct IRModule *ir, FILE *file) {
  fprintf(file, "section .data\n");

  for (int i = 0; i < ir->globals_len; i++) {
    if (ir->globals[i].value.constant.kind == Value_Int && ir->globals[i].value.constant.i == 0) continue;
    fprintf(file, "  g%ld: ", ir->globals[i].vl);
    resolveType_code(&ir->globals[i].type, file);
    fprintf(file, " ");
    resolveOperand_code(NULL, &ir->globals[i].value, file);
    fprintf(file, "\n");
  }

  fprintf(file, "section .bss\n");
  for (int i = 0; i < ir->globals_len; i++) {
    if (ir->globals[i].value.constant.kind != Value_Int || ir->globals[i].value.constant.i != 0) continue;
    fprintf(file, "  g%ld: resb %zu\n", ir->globals[i].vl, sizeOf(&ir->globals[i].type));
  }
}
