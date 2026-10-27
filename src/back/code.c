#include "back.h"

void code(struct Live *live, struct IRFunction *func, FILE *file) {
  int64_t regs[14];
  for (int i = 0; i < 14; i++) regs[i] = -1;
  int spill_slot = 0;
  fprintf(file, "%.*s:\n", func->name->length, func->name->start);

  for (int i = 0; i < func->blocks_len; i++) {
    if (i > 0) fprintf(file, ".b%d:\n", i);
    for (int j = 0; j < func->blocks[i].instructions_len; j++)
      resolveInstructions_code(regs, &spill_slot, live, &func->blocks[i].instructions[j], file);
  }
}
