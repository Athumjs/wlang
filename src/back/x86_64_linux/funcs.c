#include "x86_64_linux.h"

void x86_64_linux_functions(struct LIRFunction *funcs, size_t funcs_len, FILE *file, struct Arch *arch) {
  for (int i = 0; i < funcs_len; i++) {
    fprintf(file, "%.*s:\n", funcs[i].name->length, funcs[i].name->start);
    if (funcs->stack_size > 0) fprintf(file, "  subq $%ld, %%rsp\n", funcs->stack_size);

    for (int j = 0; j < funcs[i].blocks_len; j++) {
      if (j > 0) fprintf(file, ".Lb%d:\n", j);
      for (int k = 0; k < funcs[i].blocks[j].instructions_len; k++) {
        if (j == funcs[i].blocks_len - 1 && k == funcs[i].blocks[j].instructions_len - 1)
          if (funcs->stack_size > 0) fprintf(file, "  addq $%ld, %%rsp\n", funcs->stack_size);
        x86_64_linux_inst(&funcs[i].blocks[j].instructions[k], file, arch);
      }
    }
  }
}
