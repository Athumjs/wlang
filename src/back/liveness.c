#include "backs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static inline int max(int x, int y) {
  return x > y ? x : y;
}

static inline int min(int x, int y) {
  return x < y ? x : y;
}

static inline void bit_set(uint64_t *b1, int64_t b2) {
  b1[b2 >> 6] |= 1ULL << (b2 & 63);
} 

static inline int bit_test(uint64_t *b1, int64_t b2) {
  return (b1[b2 >> 6] >> (b2 & 63)) & 1;
}

void bit_union(uint64_t *b1, uint64_t *b2, size_t nw) {
  for (size_t i = 0; i < nw; i++) b1[i] |= b2[i];
}

void bit_sub(uint64_t *b1, uint64_t *b2, size_t nw) {
  for (size_t i = 0; i < nw; i++) b1[i] &= ~b2[i];
}

void bit_copy(uint64_t *b1, uint64_t *b2, size_t nw) {
  memcpy(b1, b2, nw * sizeof(uint64_t));
}

int bit_equal(uint64_t *b1, uint64_t *b2, size_t nw) {
  return memcmp(b1, b2, nw * sizeof(uint64_t)) == 0;
}

void live_inst(struct Live *live, uint64_t *use, uint64_t *def, int **calls, size_t *calls_len, size_t *calls_cap, int pos, struct LIRInstruction *inst) {
  if (inst->operands_len == 0) return;
  int op = 0;

  while (op < inst->operands_len - 1) {
    struct LIROperand *operand = &inst->operands[op++];
    if (operand->kind == LIR_Address) {
      if (operand->mem.is_base_vreg) {
        if (!bit_test(def, operand->mem.base_reg)) bit_set(use, operand->mem.base_reg);
        int start = live->intervals[operand->mem.base_reg].start;
        int end = live->intervals[operand->mem.base_reg].end;
        live->intervals[operand->mem.base_reg].start = min(start, pos);
        live->intervals[operand->mem.base_reg].end = max(end, pos);
      }

      if (operand->mem.is_index_vreg) {
        if (!bit_test(def, operand->mem.index_reg)) bit_set(use, operand->mem.index_reg);
        int start = live->intervals[operand->mem.index_reg].start;
        int end = live->intervals[operand->mem.index_reg].end;
        live->intervals[operand->mem.index_reg].start = min(start, pos);
        live->intervals[operand->mem.index_reg].end = max(end, pos);
      }
    }

    if (operand->kind != LIR_Vreg) continue;
    if (!bit_test(def, operand->reg)) bit_set(use, operand->reg);
    int start = live->intervals[operand->reg].start;
    int end = live->intervals[operand->reg].end;
    live->intervals[operand->reg].start = min(start, pos);
    live->intervals[operand->reg].end = max(end, pos);
    live->intervals[operand->reg].is_float = operand->is_float;
  }
  struct LIROperand *operand = &inst->operands[inst->operands_len - 1];
  if (inst->opcode == LIR_ADD || inst->opcode == LIR_SUB || inst->opcode == LIR_MUL ||
      inst->opcode == LIR_AND || inst->opcode == LIR_XOR || inst->opcode == LIR_OR ||
      inst->opcode == LIR_SHL || inst->opcode == LIR_ASR || inst->opcode == LIR_LSR) {
    if (operand->kind == LIR_Address) {
      if (operand->mem.is_base_vreg) {
        if (!bit_test(def, operand->mem.base_reg)) bit_set(use, operand->mem.base_reg);
        int start = live->intervals[operand->mem.base_reg].start;
        int end = live->intervals[operand->mem.base_reg].end;
        live->intervals[operand->mem.base_reg].start = min(start, pos);
        live->intervals[operand->mem.base_reg].end = max(end, pos);
      }

      if (operand->mem.is_index_vreg) {
        if (!bit_test(def, operand->mem.index_reg)) bit_set(use, operand->mem.index_reg);
        int start = live->intervals[operand->mem.index_reg].start;
        int end = live->intervals[operand->mem.index_reg].end;
        live->intervals[operand->mem.index_reg].start = min(start, pos);
        live->intervals[operand->mem.index_reg].end = max(end, pos);
      }
    } else if (operand->kind == LIR_Vreg) {
      if (!bit_test(def, operand->reg)) bit_set(use, operand->reg);
      bit_set(def, operand->reg);
      int start = live->intervals[operand->reg].start;
      int end = live->intervals[operand->reg].end;
      live->intervals[operand->reg].start = min(start, pos);
      live->intervals[operand->reg].end = max(end, pos);
      live->intervals[operand->reg].is_float = operand->is_float;
    }
  }

  else if (inst->opcode == LIR_COPY || inst->opcode == LIR_LOAD || inst->opcode == LIR_TRUNC ||
      inst->opcode == LIR_EXT || inst->opcode == LIR_CVT || inst->opcode == LIR_SETCC) {
    if (operand->kind == LIR_Address) {
      if (operand->mem.is_base_vreg) {
        if (!bit_test(def, operand->mem.base_reg)) bit_set(use, operand->mem.base_reg);
        int start = live->intervals[operand->mem.base_reg].start;
        int end = live->intervals[operand->mem.base_reg].end;
        live->intervals[operand->mem.base_reg].start = min(start, pos);
        live->intervals[operand->mem.base_reg].end = max(end, pos);
      }

      if (operand->mem.is_index_vreg) {
        if (!bit_test(def, operand->mem.index_reg)) bit_set(use, operand->mem.index_reg);
        int start = live->intervals[operand->mem.index_reg].start;
        int end = live->intervals[operand->mem.index_reg].end;
        live->intervals[operand->mem.index_reg].start = min(start, pos);
        live->intervals[operand->mem.index_reg].end = max(end, pos);
      }
    } else if (operand->kind == LIR_Vreg)
      bit_set(def, operand->reg);
  }

  else {
    if (operand->kind == LIR_Vreg)
      if (!bit_test(def, operand->reg)) bit_set(use, operand->reg);
  }

  if (inst->opcode == LIR_CALL) {
    if (*calls_len == *calls_cap) {
      *calls_cap *= 2;
      *calls = realloc(*calls, *calls_cap * sizeof(int));
    }
    (*calls)[(*calls_len)++] = pos;
  }

  if (operand->kind != LIR_Vreg) return;
  int start = live->intervals[operand->reg].start;
  int end = live->intervals[operand->reg].end;
  live->intervals[operand->reg].start = min(start, pos);
  live->intervals[operand->reg].end = max(end, pos);
  live->intervals[operand->reg].is_float = operand->is_float;
}

void liveness(struct Live *live, struct LIRFunction *func, struct Arena *arena) {
  live->intervals = arena_alloc(arena, func->vregs_len * sizeof(struct Interval));
  live->length = func->vregs_len;

  for (int i = 0; i < live->length; i++) {
    live->intervals[i] = (struct Interval){
      .start = INT_MAX,
      .end = 0,
      .reg = -1,
      .crosses_call = 0,
      .spill_slot = 0
    };
  }

  size_t nw = (func->vregs_len + 63) / 64;
  uint64_t *use = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *def = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *liveIn = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *liveOut = calloc(func->blocks_len * nw, sizeof(uint64_t));

  int *firstPos = calloc(func->blocks_len, sizeof(int));
  int *lastPos = calloc(func->blocks_len, sizeof(int));
  int *calls = malloc(sizeof(int));
  size_t calls_cap = 1;
  size_t calls_len = 0;
  int pos = -1;
  for (int i = 0; i < func->blocks_len; i++) {
    firstPos[i] = pos + 1;
    for (int j = 0; j < func->blocks[i].instructions_len; j++)
      live_inst(live, &use[i * nw], &def[i * nw], &calls, &calls_len, &calls_cap, ++pos, &func->blocks[i].instructions[j]);
    lastPos[i] = pos;
  }

  uint8_t change = 0;
  uint64_t *newIn = calloc(nw, sizeof(uint64_t));
  uint64_t *newOut = calloc(nw, sizeof(uint64_t));
  do {
    change = 0;
    for (int i = func->blocks_len - 1; i >= 0; i--) {
      memset(newIn, 0, nw * sizeof(uint64_t));
      memset(newOut, 0, nw * sizeof(uint64_t));
      for (int j = 0; j < func->blocks[i].sucs_len; j++)
        bit_union(newOut, &liveIn[func->blocks[i].sucs[j] * nw], nw);
      bit_copy(newIn, newOut, nw);
      bit_sub(newIn, &def[i * nw], nw);
      bit_union(newIn, &use[i * nw], nw);

      if (bit_equal(newIn, &liveIn[i * nw], nw) && bit_equal(newOut, &liveOut[i * nw], nw)) continue;
      bit_copy(&liveIn[i * nw], newIn, nw);
      bit_copy(&liveOut[i * nw], newOut, nw);
      change = 1;
    }
  } while (change);

  free(newIn);
  free(newOut);

  for (int i = 0; i < func->blocks_len; i++) {
    for (int j = 0; j < live->length; j++) {
      if (bit_test(&liveIn[i * nw], j))
        live->intervals[j].start = min(live->intervals[j].start, firstPos[i]);
      if (bit_test(&liveOut[i * nw], j))
        live->intervals[j].end = max(live->intervals[j].end, lastPos[i]);
    }
  }

  for (int i = 0; i < live->length; i++) {
    struct Interval *interval = &live->intervals[i];
    for (int j = 0; j < calls_len; j++) {
      if (interval->start < calls[j] && interval->end > calls[j])
        interval->crosses_call = 1;
    }
  }

  free(calls);
  free(firstPos);
  free(lastPos);
  free(use);
  free(def);
  free(liveIn);
  free(liveOut);
}
