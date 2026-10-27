#pragma once

#include <back/liveness.h>
#include <middle/irgen.h>
#include <stdio.h>

enum Regs {
  Reg_RAX,
  Reg_RBX,
  Reg_RCX,
  Reg_RDX,
  Reg_RSI,
  Reg_RDI,
  Reg_R8,
  Reg_R9,
  Reg_R10,
  Reg_R11,
  Reg_R12,
  Reg_R13,
  Reg_R14,
  Reg_R15
};

static const char *reg_names[14][9] = {
  [Reg_RAX] = { [1] = "al",   [2] = "ax",   [4] = "eax",  [8] = "rax"  },
  [Reg_RBX] = { [1] = "bl",   [2] = "bx",   [4] = "ebx",  [8] = "rbx"  }, 
  [Reg_RCX] = { [1] = "cl",   [2] = "cx",   [4] = "ecx",  [8] = "rcx"  },
  [Reg_RDX] = { [1] = "dl",   [2] = "dx",   [4] = "edx",  [8] = "rdx"  },
  [Reg_RSI] = { [1] = "sil",  [2] = "si",   [4] = "esi",  [8] = "rsi"  },
  [Reg_RDI] = { [1] = "dil",  [2] = "di",   [4] = "edi",  [8] = "rdi"  },
  [Reg_R8]  = { [1] = "r8b",  [2] = "r8w",  [4] = "r8d",  [8] = "r8"   },
  [Reg_R9]  = { [1] = "r9b",  [2] = "r9w",  [4] = "r9d",  [8] = "r9"   },
  [Reg_R10] = { [1] = "r10b", [2] = "r10w", [4] = "r10d", [8] = "r10"  },
  [Reg_R11] = { [1] = "r11b", [2] = "r11w", [4] = "r11d", [8] = "r11"  },
  [Reg_R12] = { [1] = "r12b", [2] = "r12w", [4] = "r12d", [8] = "r12"  },
  [Reg_R13] = { [1] = "r13b", [2] = "r13w", [4] = "r13d", [8] = "r13"  },
  [Reg_R14] = { [1] = "r14b", [2] = "r14w", [4] = "r14d", [8] = "r14"  },
  [Reg_R15] = { [1] = "r15b", [2] = "r15w", [4] = "r15d", [8] = "r15"  }
};

// code.c
void code(struct Live *live, struct IRFunction *func, FILE *file);

// global.c
void resolveGlobals_code(struct IRModule *ir, FILE *file);

// instructions.c
void resolveInstructions_code(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);

// operand.c
void resolveOperand_code(struct Live *live, struct IROperand *operand, FILE *file);

// utils.c
size_t sizeOf(struct IRType *type);
size_t alignOf(struct IRType *type);
