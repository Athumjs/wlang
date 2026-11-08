#include "lir.h"
#include "../backs.h"
#include <string.h>

struct LIRInstruction *lir_newInst(enum LIROpcodes opcode, int operands_len, struct LIRBasicBlocks *block, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct LIRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct LIRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct LIRInstruction));
    block->instructions = temp;
  }

  struct LIRInstruction inst = (struct LIRInstruction){
    .opcode = opcode,
    .operands = arena_alloc(arena, operands_len * sizeof(struct LIROperand)),
    .operands_len = operands_len
  };

  block->instructions[block->instructions_len] = inst;
  return &block->instructions[block->instructions_len++];
}

void lirop_reg(struct LIROperand *operand, struct Interval *interval, struct IRType *type, struct LIRContext *ctx) {
  operand->size = sizeOf(type);

  if (interval->reg != -1) {
    operand->kind = LIR_Register;
    operand->reg = interval->reg;
    return;
  }

  operand->kind = LIR_Address;
  if (interval->spillSlot == 0) {
    *ctx->stack_size += operand->size;
    interval->spillSlot = *ctx->stack_size;
  }
  operand->mem.base_reg = ctx->arch->stack_base_register;
  operand->mem.disp = interval->spill_param ? interval->spillSlot : -interval->spillSlot;
  operand->mem.index_reg = -1;
}
