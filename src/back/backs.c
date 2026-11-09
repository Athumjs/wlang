#include "backs.h"
#include <back/reg_alloc.h>
#include <stdlib.h>

char *backs(struct IRModule *ir, struct Program *program, const char *folder_name, uint8_t only_asm, uint8_t show_lir, uint8_t main_file) {
  struct Arch arch = {0};

#if defined(__x86_64__) || defined(_M_X64)
  x86_64_linux_define(&arch);
#endif

  struct LIRFunction *functions = arena_alloc(program->arena, ir->functions_len * sizeof(struct LIRFunction));

  struct Floats floats = (struct Floats){
    .values = malloc(2 * sizeof(struct Float)),
    .values_cap = 2,
    .values_len = 0
  };

  for (int i = 0; i < ir->functions_len; i++) {
    ssa_destruction(&ir->functions[i], program->arena);
    struct Live live = {0};
    liveness(&live, &ir->functions[i], program->arena);
    register_allocation(&live, &arch);
    struct LIRFunction func = {0};
    lirgen(&func, &arch, &ir->functions[i], program->arena, &live, &floats);
    functions[i] = func;
  }

  if (show_lir) showLIR(functions, ir->functions_len);

  char *name = NULL;
  int name_len = 0;
  FILE *file = create_as(folder_name, program->filename, &name, &name_len);

#if defined(__x86_64__) || defined(_M_X64)
  x86_64_linux_code(ir->globals, ir->globals_len, functions, ir->functions_len, &floats, file, &arch, main_file);
#endif

  free(floats.values);
  fclose(file);

  if (only_asm) {
    int len = name_len + 3;
    char *as_name = malloc(len);
    snprintf(as_name, len, "%.*s.s", name_len, name);
    return as_name;
  }

  return run_assembler(folder_name, name, name_len, file);
}
