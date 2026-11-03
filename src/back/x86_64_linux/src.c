#include "x86_64_linux.h"

void x86_64_linux_code(struct IRGlobal *globals, size_t globals_len, struct LIRFunction *funcs, size_t funcs_len, struct Floats *floats, FILE *file, struct Arch *arch) {
  fprintf(file, "// wlang\n");
  x86_64_linux_globals(globals, globals_len, file);
  fprintf(file, ".section .text\n.globl _start\n");
  x86_64_linux_functions(funcs, funcs_len, file, arch);
  fprintf(file, "_start:\n");
  fprintf(file, "  call main\n");
  fprintf(file, "  movl %%eax, %%edi\n");
  fprintf(file, "  movl $60, %%eax\n");
  fprintf(file, "  syscall\n");
  if (floats->values_len > 0) fprintf(file, ".section .rodata\n");
  for (int i = 0; i < floats->values_len; i++)
    fprintf(file, "  float_%d: %s %f\n", i, floats->values[i].is_double ? ".double" : ".float", floats->values[i].value);
}
