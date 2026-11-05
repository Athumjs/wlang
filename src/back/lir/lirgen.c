#include "lir.h" 
#include "../backs.h"
#include <stdlib.h>

void resolve_floats(struct Floats *floats, struct IRInstruction *inst) {
  for (int i = 0; i < inst->operands_len; i++) {
    if (inst->operands[i].kind != Operand_Constant) continue;
    uint8_t is_double = inst->operands[i].constant.kind == Value_Double;
    if (!is_double && inst->operands[i].constant.kind != Value_Float) continue;
    if (floats->values_len == floats->values_cap) {
      floats->values_cap *= 2;
      floats->values = realloc(floats->values, floats->values_cap * sizeof(struct Float));
    }
    floats->values[floats->values_len] = (struct Float){
      .is_double = is_double,
      .value = is_double ? inst->operands[i].constant.d : inst->operands[i].constant.f
    };
  }
}

void lirgen(struct LIRFunction *func, struct Arch *arch, struct IRFunction *irfunc, struct Arena *arena, struct Live *live, struct Floats *floats) {
  func->name = irfunc->name;
  func->blocks = arena_alloc(arena, irfunc->blocks_len * sizeof(struct LIRBasicBlocks));
  func->blocks_len = irfunc->blocks_len;
  func->stack_size = 0;

  struct LIRContext ctx = (struct LIRContext){
    .live = live,
    .arch = arch,
    .floats_len = &floats->values_len,
    .stack_size = &func->stack_size,
    .arena = arena,
    .func = irfunc
  };

  for (int i = 0; i < live->length; i++) {
    int64_t reg = live->intervals[i].reg;
    if (reg == -1) continue;

    for (int j = 0; j < arch->callee_saved_registers_len; j++) {
      if (arch->callee_saved_registers[j] != reg) continue;
      func->saved_regs |= (1ULL << reg);
      break;
    }
  }

  for (int i = 0; i < irfunc->blocks_len; i++) {
    func->blocks[i].instructions = arena_alloc(arena, irfunc->blocks[i].instructions_len * sizeof(struct LIRInstruction));
    func->blocks[i].instructions_cap = irfunc->blocks[i].instructions_len;
    func->blocks[i].instructions_len = 0;
    for (int j = 0; j < irfunc->blocks[i].instructions_len; j++) {
      resolve_floats(floats, &irfunc->blocks[i].instructions[j]);
      lirinst(&func->blocks[i], &irfunc->blocks[i].instructions[j], &ctx);
      ctx.pos++;
    }
  }
}
