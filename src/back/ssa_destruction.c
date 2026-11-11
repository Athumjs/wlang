#include "backs.h"
#include <stdlib.h>
#include <string.h>

void critic_edge(struct IRFunction *func, int64_t i_org_block, int64_t i_dst_block, struct Arena *arena) {
  if (func->blocks_len == func->blocks_cap) {
    size_t oldCap = func->blocks_cap;
    func->blocks_cap *= 2;
    struct IRBasicBlock **temp = arena_alloc(arena, func->blocks_cap * sizeof(struct IRBasicBlock *));
    memcpy(temp, func->blocks, oldCap * sizeof(struct IRBasicBlock *));
    func->blocks = temp;
  }

  struct IRBasicBlock *org_block = func->blocks[i_org_block];
  struct IRBasicBlock *dst_block = func->blocks[i_dst_block];

  struct IRBasicBlock block = (struct IRBasicBlock){
    .index = func->blocks_len,
    .instructions = arena_alloc(arena, sizeof(struct IRInstruction)),
    .instructions_cap = 1,
    .instructions_len = 1,
    .prevs = arena_alloc(arena, sizeof(struct IRBasicBlock)),
    .prevs_cap = 1,
    .prevs_len = 1,
    .reg = &func->reg,
    .term = 1
  };

  struct IRInstruction inst = (struct IRInstruction){
    .opcode = OPCODE_JMP,
    .vl = -1,
    .operands = arena_alloc(arena, sizeof(struct IROperand)),
    .operands_len = 1
  };

  inst.operands[0] = (struct IROperand){
    .kind = Operand_Block,
    .op_type = IRTYPE_NULL,
    .ref = i_dst_block
  };

  block.instructions[0] = inst;
  block.prevs[0] = func->blocks[i_org_block];
  func->blocks[func->blocks_len] = arena_alloc(arena, sizeof(struct IRBasicBlock));
  *func->blocks[func->blocks_len++] = block;

  struct IRInstruction *last_inst = &org_block->instructions[org_block->instructions_len - 1];

  if (last_inst->opcode == OPCODE_BR) {
    if (last_inst->operands[1].ref == i_dst_block)
      last_inst->operands[1].ref = block.index;
    else last_inst->operands[2].ref = block.index;
  } else last_inst->operands[0].ref = block.index;

  for (int i = 0; i < dst_block->prevs_len; i++) {
    if (dst_block->prevs[i]->index != i_org_block) continue;
    dst_block->prevs[i] = func->blocks[block.index];
    break;
  }
}

void one_stage(struct IRFunction *func, struct Arena *arena) {
  int len = func->blocks_len;
  for (int i = 0; i < len; i++) {
    struct IRBasicBlock *block = func->blocks[i];
    struct Successors sucs = get_successors(block);

    if (sucs.length <= 1) {
      free(sucs.blocks);
      continue;
    }

    int64_t targets[2];
    targets[0] = sucs.blocks[0];
    targets[1] = sucs.blocks[1];
    free(sucs.blocks);
    critic_edge(func, i, targets[0], arena);
    critic_edge(func, i, targets[1], arena);
  }
}

struct ParallelCopy {
  int64_t src_reg;
  int64_t dst_reg;
  struct IRType type;
};

void generate_inst(struct IRBasicBlock *block, int64_t dst_reg, struct IRType type, int64_t src_reg, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  memmove(&block->instructions[block->instructions_len], &block->instructions[block->instructions_len - 1], (block->instructions_len - (block->instructions_len - 1)) * sizeof(struct IRInstruction));
  struct IRInstruction inst = (struct IRInstruction){
    .opcode = OPCODE_CONST,
    .vl = dst_reg,
    .operands = arena_alloc(arena, sizeof(struct IROperand)),
    .operands_len = 1
  };
  inst.operands[0] = (struct IROperand){
    .kind = Operand_Register,
    .op_type = type,
    .ref = src_reg
  };
  block->instructions[block->instructions_len - 1] = inst;
  block->instructions_len++;
}

void two_stage(struct IRFunction *func, struct Arena *arena) {
  for (int i = 0; i < func->blocks_len; i++) {
    struct IRBasicBlock *block = func->blocks[i];
    if (block->prevs_len == 0) continue;
    for (int j = 0; j < block->prevs_len; j++) {
      struct IRBasicBlock *prev_block = block->prevs[j];
      struct ParallelCopy parallels[block->instructions_len];
      int count = 0;
      for (int k = 0; k < block->instructions_len; k++) {
        struct IRInstruction *inst = &block->instructions[k];
        if (inst->opcode != OPCODE_PHI) continue;
        for (int l = 0; l < inst->operands_len; l++) {
          struct IROperand *operand = &inst->operands[l];
          if (operand->phi.block->index != prev_block->index) {
            if (prev_block->prevs_len != 1) continue;
            if (operand->phi.block->index != prev_block->prevs[0]->index) continue;
          }

          if (operand->phi.value == inst->vl) continue;
          parallels[count].dst_reg = inst->vl;
          parallels[count].src_reg = operand->phi.value;
          parallels[count].type = operand->op_type;
          count++;
        }
      }
      int in_degree[count];
      for (int c = 0; c < count; c++) in_degree[c] = 0;
      for (int k = 0; k < count; k++) {
        for (int l = 0; l < count; l++) {
          if (k == l) continue;
          if (parallels[k].dst_reg != parallels[l].src_reg) continue;
          in_degree[k]++;
        }
      }
      int ready[count];
      int ready_count = 0;
      for (int c = 0; c < count; c++) {
        if (in_degree[c] != 0) continue;
        ready[ready_count++] = c;
      }
      uint8_t emmited[count];
      for (int c = 0; c < count; c++) emmited[c] = 0;
      int total_emmited = 0;
      while (total_emmited < count) {
        if (ready_count > 0) {
          int index = ready[--ready_count];
          generate_inst(prev_block, parallels[index].dst_reg, parallels[index].type, parallels[index].src_reg, arena);
          emmited[index] = 1;
          total_emmited++;
          for (int k = 0; k < count; k++) {
            if (emmited[k] || parallels[index].dst_reg != parallels[k].src_reg) continue;
            in_degree[k]--;
            if (in_degree[k] != 0) continue;
            ready[ready_count++] = k;
          }
        } else {
          for (int k = 0; k < count; k++) {
            if (emmited[k]) continue;
            int tmp = func->reg++;
            generate_inst(prev_block, tmp, parallels[k].type, parallels[k].src_reg, arena);
            for (int l = 0; l < count; l++) {
              if (emmited[l]) continue;
              if (parallels[l].src_reg != parallels[k].src_reg) continue;
              parallels[l].src_reg = tmp;
            }
            ready[ready_count++] = k;
            break;
          }
        }
      }
    }
    int write_index = 0;
    for (int j = 0; j < block->instructions_len; j++) {
      struct IRInstruction *inst = &block->instructions[j];
      if (inst->opcode == OPCODE_PHI) continue;
      block->instructions[write_index++] = *inst;
    }
    block->instructions_len = write_index;
  }
}

void ssa_destruction(struct IRFunction *func, struct Arena *arena) {
  one_stage(func, arena);
  two_stage(func, arena);
}
