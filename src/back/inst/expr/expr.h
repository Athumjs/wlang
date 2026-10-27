#pragma once

#include "../inst.h"

// binary.c
void resolveAdd_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);
void resolveSub_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);
void resolveMul_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);

// primary.c
void resolveConst_inst(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);

// cast.c
void resolveTrunc_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file);
void resolveZext_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file);
void resolveSext_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file);
