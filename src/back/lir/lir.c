#include "lir.h"
#include <stdlib.h>
#include <string.h>

struct LIRInstruction *lir_newInst(enum LIROpcodes opcode, size_t operands_len, struct LIRBasicBlocks *block, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct LIRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct LIRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct LIRInstruction));
    block->instructions = temp;
  }

  block->instructions[block->instructions_len++] = (struct LIRInstruction){
    opcode,
    0,
    arena_alloc(arena, operands_len * sizeof(struct LIROperand)),
    0
  };

  return &block->instructions[block->instructions_len - 1];
}

void lir_addOperandReg(uint8_t isVreg, int64_t reg, struct IRType *type, uint8_t is_float, struct LIRInstruction *inst, struct LIRContext *ctx) {
  inst->operands[inst->operands_len++] = (struct LIROperand){
    .kind = isVreg ? LIR_Vreg : LIR_Register,
    .reg = isVreg && reg == -1 ? ctx->vregs_len++ : reg,
    .size = sizeOf(type),
    .is_float = is_float
  };
}

/**
 * type = 1 : Float | type = 2 Global | type >= 3 Address
**/
void lir_addOperandAddress(uint8_t type, int64_t base_reg, uint8_t is_base_vreg, int64_t index_reg, uint8_t is_index_vreg, int64_t disp, uint8_t scale, struct IRType *op_type, struct LIRInstruction *inst, struct LIRContext *ctx) {
  struct LIROperand operand = (struct LIROperand){
    .mem.base_reg = base_reg,
    .mem.is_base_vreg = is_base_vreg,
    .mem.index_reg = index_reg,
    .mem.is_index_vreg = is_index_vreg,
    .mem.disp = disp,
    .mem.scale = scale,
    .size = sizeOf(op_type)
  };

  if (type == 1) {
    operand.kind = LIR_Float;
    operand.is_float = 1;
  }

  else if (type == 2)
    operand.kind = LIR_Global;
  else operand.kind = LIR_Address;

  inst->operands[inst->operands_len++] = operand;
}

void lir_addOperandConst(int64_t value, struct IRType *type, struct LIRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct LIROperand){
    .kind = LIR_Immediate,
    .imm = value,
    .size = sizeOf(type)
  };
}

/**
 * type = 1 : Global | type >= 2 Label
**/
void lir_addOperandLabel(uint8_t type, struct String *str, struct IRType *op_type, struct LIRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct LIROperand){
    .kind = type == 1 ? LIR_Global : LIR_Label,
    .label = malloc(str->length + 1),
    .size = sizeOf(op_type)
  };
  snprintf(inst->operands[inst->operands_len - 1].label, str->length + 1, "%.*s", str->length, str->start);
}

uint8_t is_float(struct IRType *type) {
  if (type->kind != IRTYPEK_OTHER) return 0;
  if (type->other != IRTYPE_f32 && type->other != IRTYPE_f64) return 0;
  return 1;
}
