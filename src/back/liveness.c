#include <back/liveness.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int max(int x, int y) {
  return x > y ? x : y;
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

void resolveOperands(uint64_t *def, uint64_t *use, struct IROperand *operand) {
  if (operand->kind == Operand_Call) {
    resolveOperands(def, use, operand->call.operand);
    for (int i = 0; i < operand->call.args_len; i++)
      resolveOperands(def, use, &operand->call.args[i]);
    return;
  }

  if (operand->kind != Operand_Register) return;
  if (!bit_test(def, operand->ref)) bit_set(use, operand->ref);
}

void verifyOperands(struct Live *live, struct IROperand *operand, int pos) {
  if (operand->kind == Operand_Call) {
    verifyOperands(live, operand->call.operand, pos);
    for (int i = 0; i < operand->call.args_len; i++)
      verifyOperands(live, &operand->call.args[i], pos);
    return;
  }

  if (operand->kind != Operand_Register) return;
  live->intervals[operand->ref].end = max(live->intervals[operand->ref].end, pos);
}

void liveness(struct Live *live, struct IRFunction *func, struct Arena *arena) {
  live->intervals = arena_alloc(arena, func->reg * sizeof(struct Interval));
  live->length = func->reg;
  size_t nw = (func->reg + 63) / 64;
  uint64_t *use = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *def = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *liveIn = calloc(func->blocks_len * nw, sizeof(uint64_t));
  uint64_t *liveOut = calloc(func->blocks_len * nw, sizeof(uint64_t));

  for (int i = 0; i < func->params_len; i++) {
    bit_set(&def[0], func->params[i].vl);
    live->intervals[func->params[i].vl] = (struct Interval){
      .vreg = func->params[i].vl,
      .start = 0,
      .end = 0
    };
  }

  int pos = 0;
  int *firstPos = calloc(func->blocks_len, sizeof(int));
  int *lastPos = calloc(func->blocks_len, sizeof(int));
  for (int i = 0; i < func->blocks_len; i++) {
    firstPos[i] = pos + 1;
    for (int j = 0; j < func->blocks[i].instructions_len; j++) {
      pos++;
      for (int k = 0; k < func->blocks[i].instructions[j].operands_len; k++)
        resolveOperands(&def[i * nw], &use[i * nw], &func->blocks[i].instructions[j].operands[k]);

      if (func->blocks[i].instructions[j].vl == -1) continue;
      bit_set(&def[i * nw], func->blocks[i].instructions[j].vl);
      live->intervals[func->blocks[i].instructions[j].vl] = (struct Interval){
        .vreg = func->blocks[i].instructions[j].vl,
        .start = pos,
        .end = pos
      };
    }
    lastPos[i] = pos;
  }

  uint8_t change = 0;

  do {
    change = 0;
    uint64_t *newIn = calloc(nw, sizeof(uint64_t));
    uint64_t *newOut = calloc(nw, sizeof(uint64_t));
    for (int i = 0; i < func->blocks_len; i++) {
      memset(newIn, 0, nw * sizeof(uint64_t));
      memset(newOut, 0, nw * sizeof(uint64_t));
      struct IRInstruction *inst = &func->blocks[i].instructions[func->blocks[i].instructions_len - 1];

      if (inst->opcode == OPCODE_BR) {
        bit_copy(newOut, &liveIn[inst->operands[1].ref * nw], nw);
        bit_union(newOut, &liveIn[inst->operands[2].ref * nw], nw);

        for (int j = 1; j <= 2; j++) {
          for (int k = 0; k < func->blocks[inst->operands[j].ref].instructions_len; k++) {
            if (func->blocks[inst->operands[j].ref].instructions[k].opcode != OPCODE_PHI) continue;
            for (int l = 0; l < func->blocks[inst->operands[j].ref].instructions[k].operands_len; l++) {
              struct IROperand *operand = &func->blocks[inst->operands[j].ref].instructions[k].operands[l];
              if (operand->phi.block->index == i) bit_set(newOut, operand->phi.value);
            }
          }
        }
      }

      else if (inst->opcode == OPCODE_JMP) {
        bit_copy(newOut, &liveIn[inst->operands[0].ref * nw], nw);
        for (int j = 0; j < func->blocks[inst->operands[0].ref].instructions_len; j++) {
          if (func->blocks[inst->operands[0].ref].instructions[j].opcode != OPCODE_PHI) continue;
          for (int k = 0; k < func->blocks[inst->operands[0].ref].instructions[j].operands_len; k++) {
            struct IROperand *operand = &func->blocks[inst->operands[0].ref].instructions[j].operands[k];
            if (operand->phi.block->index == i) bit_set(newOut, operand->phi.value);
          }
        }
      }

      bit_copy(newIn, newOut, nw);
      bit_sub(newIn, &def[i * nw], nw);
      bit_union(newIn, &use[i * nw], nw);

      if (!bit_equal(newIn, &liveIn[i * nw], nw) || !bit_equal(newOut, &liveOut[i * nw], nw)) {
        bit_copy(&liveIn[i * nw], newIn, nw);
        bit_copy(&liveOut[i * nw], newOut, nw);
        change = 1;
      }
    }
    free(newIn);
    free(newOut);
  } while (change);

  pos = 0;
  for (int i = 0; i < func->blocks_len; i++) {
    for (int j = 0; j < func->blocks[i].instructions_len; j++) {
      pos++;
      for (int k = 0; k < func->blocks[i].instructions[j].operands_len; k++)
        verifyOperands(live, &func->blocks[i].instructions[j].operands[k], pos);
    }
  }

  for (int i = 0; i < func->blocks_len; i++) {
    for (int j = 0; j < func->reg; j++) {
      if (bit_test(&liveIn[i * nw], j))
        live->intervals[j].end = max(live->intervals[j].end, firstPos[i]);

      if (bit_test(&liveOut[i * nw], j))
        live->intervals[j].end = max(live->intervals[j].end, lastPos[i]);
    }
  }

  for (int i = 0; i < live->length; i++) {
    printf("%%%ld start: %d | end: %d\n", live->intervals[i].vreg, live->intervals[i].start, live->intervals[i].end);
  }

  free(firstPos);
  free(lastPos);
  free(use);
  free(def);
  free(liveIn);
  free(liveOut);
}
