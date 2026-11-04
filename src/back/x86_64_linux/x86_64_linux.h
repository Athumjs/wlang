#pragma once

#include "../backs.h"

static const char *reg_names[32][9] = {
    // Registradores Inteiros Clássicos (0 a 7)
    [0] = { [1] = "al",   [2] = "ax",   [4] = "eax",  [8] = "rax" },
    [1] = { [1] = "bl",   [2] = "bx",   [4] = "ebx",  [8] = "rbx" },
    [2] = { [1] = "cl",   [2] = "cx",   [4] = "ecx",  [8] = "rcx" },
    [3] = { [1] = "dl",   [2] = "dx",   [4] = "edx",  [8] = "rdx" },
    [4] = { [1] = "sil",  [2] = "si",   [4] = "esi",  [8] = "rsi" },
    [5] = { [1] = "dil",  [2] = "di",   [4] = "edi",  [8] = "rdi" },
    [6] = { [1] = "rbp",  [2] = "rbp",   [4] = "rbp",  [8] = "rbp" }, // Reservado (FP)
    [7] = { [1] = "rsp",  [2] = "rsp",   [4] = "rsp",  [8] = "rsp" }, // Reservado (SP)

    // Registradores Inteiros Estendidos R8-R15 (8 a 15)
    [8]  = { [1] = "r8b",  [2] = "r8w",  [4] = "r8d",  [8] = "r8"  },
    [9]  = { [1] = "r9b",  [2] = "r9w",  [4] = "r9d",  [8] = "r9"  },
    [10] = { [1] = "r10b", [2] = "r10w", [4] = "r10d", [8] = "r10" },
    [11] = { [1] = "r11b", [2] = "r11w", [4] = "r11d", [8] = "r11" },
    [12] = { [1] = "r12b", [2] = "r12w", [4] = "r12d", [8] = "r12" },
    [13] = { [1] = "r13b", [2] = "r13w", [4] = "r13d", [8] = "r13" },
    [14] = { [1] = "r14b", [2] = "r14w", [4] = "r14d", [8] = "r14" },
    [15] = { [1] = "r15b", [2] = "r15w", [4] = "r15d", [8] = "r15" },

    // Registradores de Ponto Flutuante XMM0-XMM15 (16 a 31)
    [16] = { [4] = "xmm0",  [8] = "xmm0"  },
    [17] = { [4] = "xmm1",  [8] = "xmm1"  },
    [18] = { [4] = "xmm2",  [8] = "xmm2"  },
    [19] = { [4] = "xmm3",  [8] = "xmm3"  },
    [20] = { [4] = "xmm4",  [8] = "xmm4"  },
    [21] = { [4] = "xmm5",  [8] = "xmm5"  },
    [22] = { [4] = "xmm6",  [8] = "xmm6"  },
    [23] = { [4] = "xmm7",  [8] = "xmm7"  },
    [24] = { [4] = "xmm8",  [8] = "xmm8"  },
    [25] = { [4] = "xmm9",  [8] = "xmm9"  },
    [26] = { [4] = "xmm10", [8] = "xmm10" },
    [27] = { [4] = "xmm11", [8] = "xmm11" },
    [28] = { [4] = "xmm12", [8] = "xmm12" },
    [29] = { [4] = "xmm13", [8] = "xmm13" },
    [30] = { [4] = "xmm14", [8] = "xmm14" },
    [31] = { [4] = "xmm15", [8] = "xmm15" }
};

// globals.c
void x86_64_linux_globals(struct IRGlobal *globals, size_t globals_len, FILE *file);

// funcs.c
void x86_64_linux_functions(struct LIRFunction *funcs, size_t funcs_len, FILE *file, struct Arch *arch);

// inst.c
void x86_64_linux_inst(struct LIRInstruction *inst, FILE *file, struct Arch *arch);

// operand.c
void x86_64_linux_operand(struct LIROperand *operand, FILE *file);

// extension.c
void x86_64_linux_extension(uint64_t ext, uint8_t size, FILE *file);

// util.c
char *x86_64_linux_inst_size(uint8_t size, uint8_t is_float);
char *x86_64_linux_conds(enum LIRCmp cmp);
