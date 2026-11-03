#pragma once

#include <back/lir.h>
#include <back/backs.h>
#include <back/reg_alloc.h>

struct LIRContext {
  struct Arch *arch;
  struct Live *live;
  struct Arena *arena;
  size_t *stack_size;
  size_t *floats_len;
  int pos;
};

// lirinst.c
void lirinst(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx);

// liroperand.c
void liroperand(struct LIROperand *operand, struct IROperand *iroperand, struct LIRContext *ctx);

// lirutil.c
struct LIRInstruction *lir_newInst(enum LIROpcodes opcode, int operands_len, struct LIRBasicBlocks *block, struct Arena *arena, struct IRType *type);
void lirop_reg(struct LIROperand *operand, struct Interval *interval, struct LIRContext *ctx);
