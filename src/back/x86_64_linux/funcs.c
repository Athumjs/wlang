#include "x86_64_linux.h"

static void emit_prologue(struct LIRFunction *func, FILE *file, struct Arch *arch) { 
  if (func->stack_size > 0) {
    fprintf(file, "  pushq %%rbp\n");
    fprintf(file, "  movq %%rsp, %%rbp\n");
  }

  for (int i = 0; i < arch->callee_saved_registers_len; i++) {
    int64_t reg = arch->callee_saved_registers[i];
    if (!(func->saved_regs & (1ULL << reg))) continue;
    fprintf(file, "  pushq %%%s\n", reg_names[reg][8]);
  } 

  if (func->stack_size > 0)
    if (func->stack_size >= 120 || func->has_call) fprintf(file, "  subq $%ld, %%rsp\n", (func->stack_size + 15) & ~15);
}

static void emit_epilogue(struct LIRFunction *func, FILE *file, struct Arch *arch) {  
  for (int i = arch->callee_saved_registers_len - 1; i >= 0; i--) {
    int64_t reg = arch->callee_saved_registers[i];
    if (!(func->saved_regs & (1ULL << reg))) continue;
    if (func->stack_size >= 120 || func->has_call) fprintf(file, "  movq -%d(%%rbp), %%%s\n", i * 8, reg_names[reg][8]);
    else fprintf(file, "  popq %%%s\n", reg_names[reg][8]);
  }

  if (func->stack_size > 0) {
    if (func->stack_size >= 120 || func->has_call) fprintf(file, "  leave\n");
    else fprintf(file, "  popq %%rbp\n");
  }
}

void x86_64_linux_functions(struct LIRFunction *funcs, size_t funcs_len, FILE *file, struct Arch *arch) {
  for (int i = 0; i < funcs_len; i++) {
    fprintf(file, "%.*s:\n", funcs[i].name->length, funcs[i].name->start); 
    emit_prologue(&funcs[i], file, arch);

    for (int j = 0; j < funcs[i].blocks_len; j++) {
      if (j > 0) fprintf(file, ".Lb%d:\n", j);
      for (int k = 0; k < funcs[i].blocks[j].instructions_len; k++) {
        if (k == funcs[i].blocks[j].instructions_len - 1 && funcs[i].blocks[j].instructions[k].opcode == LIR_RET)
          emit_epilogue(&funcs[i], file, arch);
        x86_64_linux_inst(&funcs[i].blocks[j].instructions[k], file, arch);
      }
    }
  }
}
