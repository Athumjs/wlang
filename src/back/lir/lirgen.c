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
  func->public = irfunc->public;

  struct LIRContext ctx = (struct LIRContext){
    .live = live,
    .arch = arch,
    .floats_len = &floats->values_len,
    .stack_size = &func->stack_size,
    .arena = arena,
    .func = irfunc
  };

  int saved_regs = 0;
  for (int i = 0; i < live->length; i++) {
    int64_t reg = live->intervals[i].reg;
    if (reg == -1) continue;

    for (int j = 0; j < arch->callee_saved_registers_len; j++) {
      if (arch->callee_saved_registers[j] != reg) continue;
      func->saved_regs |= (1ULL << reg);
      break;
    }
  }

  for (int i = arch->callee_saved_registers_len - 1; i >= 0; i--) {
    int64_t reg = arch->callee_saved_registers[i];
    if (!(func->saved_regs & (1ULL << reg))) continue;
    saved_regs++;
  }

  func->stack_size = saved_regs * 8;
  func->blocks[0].instructions = arena_alloc(arena, irfunc->blocks[0].instructions_len * sizeof(struct LIRInstruction));
  func->blocks[0].instructions_cap = irfunc->blocks[0].instructions_len;
  func->blocks[0].instructions_len = 0;

  for (int i = 0; i < irfunc->params_len; i++) {
    struct Interval *interval = &live->intervals[irfunc->params[i].vl];
    if (interval->is_float) {
      if (i >= arch->arg_fregisters_len) {
        interval->spillSlot = 16 + ((i - arch->arg_fregisters_len) * 8);
        interval->spill_param = 1;
      } else interval->reg = arch->arg_registers[i + arch->arg_iregisters_len];
    } else {
      if (i >= arch->arg_iregisters_len) {
        interval->spillSlot = 16 + ((i - arch->arg_iregisters_len) * 8);
        interval->spill_param = 1;
      } else interval->reg = arch->arg_registers[i];
    }

    if (interval->spill_force && interval->reg != -1) {
      struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, &func->blocks[0], arena);
      inst->operands[0].kind = LIR_Register;
      inst->operands[0].reg = interval->reg;
      inst->operands[0].size = sizeOf(&irfunc->params[i].type);
      interval->reg = -1;
      lirop_reg(&inst->operands[1], interval, &irfunc->params[i].type, &ctx);
    }
  }

  for (int i = 0; i < irfunc->blocks_len; i++) {
    if (i > 0) {
      func->blocks[i].instructions = arena_alloc(arena, irfunc->blocks[i].instructions_len * sizeof(struct LIRInstruction));
      func->blocks[i].instructions_cap = irfunc->blocks[i].instructions_len;
      func->blocks[i].instructions_len = 0;
    } 
    for (int j = 0; j < irfunc->blocks[i].instructions_len; j++) {
      resolve_floats(floats, &irfunc->blocks[i].instructions[j]);
      lirinst(&func->blocks[i], &irfunc->blocks[i].instructions[j], &ctx);
      ctx.pos++;
    }
  }

  func->stack_size -= saved_regs * 8;
  func->has_call = ctx.has_call;
  if (func->has_call) func->stack_size += 8;
}
