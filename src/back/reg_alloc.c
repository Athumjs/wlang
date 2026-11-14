#include "backs.h"
#include "lir/lir.h"
#include <limits.h>
#include <stdlib.h>

int cmp_intervals_start(const void *x, const void *y) {
  return (*(struct Interval **)x)->start - (*(struct Interval **)y)->start;
}

void actives_clean(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs) {
  for (size_t i = 0; i < *actives_len; i++) {
    if (actives[i]->end >= interval->start) continue;
    free_regs[actives[i]->reg] = 1;
    actives[i] = actives[*actives_len - 1];
    (*actives_len)--;
    i--;
  }
}

void alloc_ireg(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs, size_t regs_len, size_t *stack_size, uint64_t *saved_regs, struct Arch *arch) { 
  if (interval->crosses_call) {
    for (size_t i = 0; i < arch->callee_saved_registers_len; i++) {
      int64_t reg = arch->callee_saved_registers[i];
      if (!free_regs[reg]) continue;
      free_regs[reg] = 0;
      interval->reg = reg;
      actives[(*actives_len)++] = interval;
      *saved_regs |= (1ULL << reg);
      return;
    }
    interval->reg = -1;
    *stack_size += 8;
    interval->spill_slot = *stack_size;
    return;
  }

  for (size_t i = 0; i < regs_len; i++) {
    if (!free_regs[i]) continue;
    free_regs[i] = 0;
    interval->reg = i;
    actives[(*actives_len)++] = interval;
    for (int j = 0; j < arch->callee_saved_registers_len; j++) {
      int64_t reg = arch->callee_saved_registers[j];
      if (reg != i) continue;
      *saved_regs |= (1ULL << reg);
    }
    return;
  }

  struct Interval *spill = interval;
  int spill_index = -1;

  for (size_t i  = 0; i < *actives_len; i++) {
    if (actives[i]->end <= spill->end) continue;
    if (actives[i]->is_float) continue;
    spill = actives[i];
    spill_index = i;
  }

  if (spill != interval) {
    interval->reg = spill->reg;
    spill->reg = -1;
    *stack_size += 8;
    spill->spill_slot = *stack_size;
    actives[spill_index] = interval;
  } else {
    interval->reg = -1;
    *stack_size += 8;
    interval->spill_slot = *stack_size;
  }
}

void alloc_freg(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs, size_t iregs_len, size_t fregs_len, size_t *stack_size) {
  if (interval->crosses_call) {
    interval->reg = -1;
    *stack_size += 8;
    interval->spill_slot = *stack_size;
    return;
  }

  for (size_t i = iregs_len; i < iregs_len + fregs_len; i++) {
    if (!free_regs[i]) continue;
    free_regs[i] = 0;
    interval->reg = i;
    actives[(*actives_len)++] = interval;
    return;
  }

  struct Interval *spill = interval;
  int spill_index = -1;

  for (size_t i  = 0; i < *actives_len; i++) {
    if (actives[i]->end <= spill->end) continue;
    if (!actives[i]->is_float) continue;
    spill = actives[i];
    spill_index = i;
  }

  if (spill != interval) {
    interval->reg = spill->reg;
    spill->reg = -1;
    *stack_size += 8;
    spill->spill_slot = *stack_size;
    actives[spill_index] = interval;
  } else {
    interval->reg = -1;
    *stack_size += 8;
    interval->spill_slot = *stack_size;
  }
}

void register_allocation(struct Live *live, struct LIRFunction *func, struct Arch *arch, struct Arena *arena) {
  if (live->length == 0) return;
  struct Interval *intervals[live->length];
  for (int i = 0; i < live->length; i++)
    intervals[i] = &live->intervals[i];
  qsort(intervals, live->length, sizeof(struct Interval *), cmp_intervals_start);

  uint8_t *free_regs = malloc((arch->iregisters_len + arch->fregisters_len) * sizeof(uint8_t));

  for (size_t i = 0; i < arch->iregisters_len + arch->fregisters_len; i++)
    free_regs[i] = !arch->registers[i].reserve;

  struct Interval **actives = calloc(arch->iregisters_len + arch->fregisters_len, sizeof(struct Interval *));
  size_t actives_len = 0;

  for (int i = 0; i < live->length; i++) {
    actives_clean(actives, &actives_len, intervals[i], free_regs); 
    if (intervals[i]->start == INT_MAX) continue;

    if (intervals[i]->is_float)
      alloc_freg(actives, &actives_len, intervals[i], free_regs, arch->iregisters_len, arch->fregisters_len, &func->stack_size);
    else alloc_ireg(actives, &actives_len, intervals[i], free_regs, arch->iregisters_len, &func->stack_size, &func->saved_regs, arch); 
  }

  for (int i = 0; i < func->blocks_len; i++) {
    for (int j = 0; j < func->blocks[i].instructions_len; j++) {
      struct LIRInstruction *inst = &func->blocks[i].instructions[j];
      for (int k = 0; k < inst->operands_len; k++) {
        struct LIROperand *operand = &inst->operands[k];
        if (operand->kind == LIR_Address) {
          if (!operand->mem.is_base_vreg) continue;
          struct Interval *interval = &live->intervals[operand->mem.base_reg];
          if (interval->reg != -1) {
            operand->mem.base_reg = interval->reg;
            operand->mem.is_base_vreg = 0;
          } else {
            struct LIRInstruction *op = lir_newInst(LIR_COPY, 2, &func->blocks[i], arena);
            op->operands[op->operands_len++] = (struct LIROperand){
              .kind = LIR_Address,
              .mem.base_reg = arch->stack_base_register,
              .mem.is_base_vreg = 0,
              .mem.index_reg = -1,
              .mem.is_index_vreg = 0,
              .mem.disp = -interval->spill_slot,
              .mem.scale = 0
            };
            op->operands[op->operands_len++] = (struct LIROperand){
              .kind = LIR_Register,
              .reg = arch->tmp_iregister
            };
            operand->kind = LIR_Register;
            operand->reg = arch->tmp_iregister;
          }
          if (operand->mem.index_reg == -1) continue;
          if (!operand->mem.is_index_vreg) continue;
          interval = &live->intervals[operand->mem.index_reg];
          if (interval->reg != -1) {
            operand->mem.base_reg = interval->reg;
            operand->mem.is_base_vreg = 0;
          } else {
            struct LIRInstruction *op = lir_newInst(LIR_COPY, 2, &func->blocks[i], arena);
            op->operands[op->operands_len++] = (struct LIROperand){
              .kind = LIR_Address,
              .mem.base_reg = arch->stack_base_register,
              .mem.is_base_vreg = 0,
              .mem.index_reg = -1,
              .mem.is_index_vreg = 0,
              .mem.disp = -interval->spill_slot,
              .mem.scale = 0
            };
            op->operands[op->operands_len++] = (struct LIROperand){
              .kind = LIR_Register,
              .reg = arch->tmp_iregister
            };
            operand->kind = LIR_Register;
            operand->reg = arch->tmp_iregister;
          }
        }

        if (operand->kind != LIR_Vreg) continue;
        struct Interval *interval = &live->intervals[operand->reg];
        if (interval->reg != -1) {
          operand->kind = LIR_Register;
          operand->reg = interval->reg;
        } else {
          operand->kind = LIR_Address;
          operand->mem.base_reg = arch->stack_base_register;
          operand->mem.is_base_vreg = 0;
          operand->mem.index_reg = -1;
          operand->mem.is_index_vreg = 0;
          operand->mem.disp = -interval->spill_slot;
          operand->mem.scale = 0;
        }
      }
    }
  }

  free(free_regs);
  free(actives);
}
