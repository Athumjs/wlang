#include "lir.h"
#include <stdlib.h>

void operand_float(struct IROperand *operand, struct Floats *floats) {
  if (operand->kind != Operand_Constant) return;
  uint8_t is_double = operand->op_type.other == IRTYPE_f64;
  if (operand->op_type.kind != IRTYPEK_OTHER || (!is_double && operand->op_type.other != IRTYPE_f32)) return;
  if (floats->values_len == floats->values_cap) {
    floats->values_cap *= 2;
    floats->values = realloc(floats->values, floats->values_cap * sizeof(struct Float));
  }
  floats->values[floats->values_len++] = (struct Float){
    is_double,
    .value = is_double ? operand->constant.d : operand->constant.f
  };
}

void resolve_floats(struct IRInstruction *inst, struct Floats *floats) {
  for (int i = 0; i < inst->operands_len; i++) {
    if (inst->operands[i].kind == Operand_Call) {
      operand_float(inst->operands[i].call.operand, floats);
      for (int j = 0; j < inst->operands[i].call.args_len; j++)
        operand_float(&inst->operands[i].call.args[j], floats);
      continue;
    }

    operand_float(&inst->operands[i], floats);
  }
}

void lirgen(struct LIRFunction *func, struct Arch *arch, struct IRFunction *irfunc, struct Arena *arena, struct Floats *floats) {
  struct LIRContext ctx = {
    .stack_len = 0,
    .floats_len = floats->values_len,
    .vregs_len = irfunc->reg,
    .has_call = 0,
    .arena = arena,
    .arch = arch,
    .func = irfunc
  };

  func->name = irfunc->name;
  func->blocks = arena_alloc(arena, irfunc->blocks_len * sizeof(struct LIRBasicBlocks));
  func->blocks_len = irfunc->blocks_len;
  func->public = irfunc->public;

  func->blocks[0].instructions = arena_alloc(arena, (irfunc->params_len + 1) * sizeof(struct LIRInstruction));
  func->blocks[0].instructions_cap = irfunc->params_len + 1;
  func->blocks[0].instructions_len = 0;

  for (int i = 0; i < irfunc->params_len; i++) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, &func->blocks[0], arena);
    int64_t reg = -1;
    if (irfunc->params[i].type.kind == IRTYPEK_OTHER &&
        (irfunc->params[i].type.other == IRTYPE_f32 || irfunc->params[i].type.other == IRTYPE_f64)) {
      if (i < arch->arg_fregisters_len) reg = arch->arg_registers[i + arch->arg_iregisters_len];
    } else
      if (i < arch->arg_iregisters_len) reg = arch->arg_registers[i];
    if (reg != -1) lir_addOperandReg(0, reg, &irfunc->params[i].type, is_float(&irfunc->params[i].type), inst, &ctx);
    else lir_addOperandAddress(3, arch->stack_base_register, 0, -1, 0, 16 + (i * 8), 0, &irfunc->params[i].type, inst, &ctx);
    lir_addOperandReg(1, irfunc->params[i].vl, &irfunc->params[i].type, is_float(&irfunc->params[i].type), inst, &ctx);
  }

  for (int i = 0; i < irfunc->blocks_len; i++) {
    if (i > 0) {
      func->blocks[i].instructions = arena_alloc(arena, 5 * sizeof(struct LIRInstruction));
      func->blocks[i].instructions_cap = 5;
      func->blocks[i].instructions_len = 0;
    }

    for (int j = 0; j < irfunc->blocks[i]->instructions_len; j++) {
      resolve_floats(&irfunc->blocks[i]->instructions[j], floats);
      lir_inst(&func->blocks[i], &irfunc->blocks[i]->instructions[j], &ctx);
    }
  }

  func->stack_size = ctx.stack_len;
  func->vregs_len = ctx.vregs_len;
  func->has_call = ctx.has_call;
}
