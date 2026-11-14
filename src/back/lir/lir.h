#pragma once

#include "../backs.h"

struct LIRContext {
  size_t stack_len;
  size_t vregs_len;
  size_t floats_len;
  struct Arch *arch;
  struct Arena *arena;
  struct IRFunction *func;
  uint8_t has_call;
};

// lir.c
struct LIRInstruction *lir_newInst(enum LIROpcodes opcode, size_t operands_len, struct LIRBasicBlocks *block, struct Arena *arena);
void lir_addOperandReg(uint8_t isVreg, int64_t reg, struct IRType *type, uint8_t is_float, struct LIRInstruction *inst, struct LIRContext *ctx);
void lir_addOperandAddress(uint8_t type, int64_t base_reg, uint8_t is_base_vreg, int64_t index_reg, uint8_t is_index_vreg, int64_t disp, uint8_t scale, struct IRType *op_type, struct LIRInstruction *inst, struct LIRContext *ctx);
void lir_addOperandConst(int64_t value, struct IRType *type, struct LIRInstruction *inst);
void lir_addOperandLabel(uint8_t type, struct String *str, struct IRType *op_type, struct LIRInstruction *inst);
uint8_t is_float(struct IRType *type);

// inst.c
void lir_inst(struct LIRBasicBlocks *block, struct IRInstruction *inst, struct LIRContext *ctx);

// operand.c
void lir_operand(struct LIRInstruction *inst, struct IROperand *operand, struct LIRContext *ctx);
