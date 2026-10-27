#pragma once

#include "../back.h"

// instructions.c
int defineReg_code(int64_t *regs, int *spill_slot, struct Live *live, int64_t value, FILE *file);

// stmt.c
int resolveStmt_code(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file);

// expr/expr.c
int resolveExpr_code(int64_t *regs, int *spill_slot, struct Live *live, struct IRInstruction *inst, FILE *file);
