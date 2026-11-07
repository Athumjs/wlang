#include "backs.h"
#include <stdlib.h>
#include <string.h>

struct Successors {
  int64_t *blocks;
  size_t length;
};

struct Successors get_successors(struct IRBasicBlock *block) {
  struct Successors sucs = {0};
  struct IRInstruction *last_inst = &block->instructions[block->instructions_len - 1];

  if (last_inst->opcode == OPCODE_BR) {
    sucs.blocks = malloc(2 * sizeof(int64_t));
    sucs.length = 2;
    sucs.blocks[0] = last_inst->operands[1].ref;
    sucs.blocks[1] = last_inst->operands[2].ref;
  } else if (last_inst->opcode == OPCODE_JMP) {
    sucs.blocks = malloc(sizeof(int64_t));
    sucs.length = 1;
    sucs.blocks[0] = last_inst->operands[0].ref;
  }

  return sucs;
}

void critic_edge(struct IRFunction *func, int64_t i_org_block, int64_t i_dst_block, struct Arena *arena) {
  if (func->blocks_len == func->blocks_cap) {
    size_t oldCap = func->blocks_cap;
    func->blocks_cap *= 2;
    struct IRBasicBlock *temp = arena_alloc(arena, func->blocks_cap * sizeof(struct IRBasicBlock));
    memcpy(temp, func->blocks, oldCap * sizeof(struct IRBasicBlock));
    func->blocks = temp;
  }

  struct IRBasicBlock *org_block = &func->blocks[i_org_block];
  struct IRBasicBlock *dst_block = &func->blocks[i_dst_block];

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
  block.prevs[0] = &func->blocks[i_org_block];
  func->blocks[func->blocks_len++] = block;

  struct IRInstruction *last_inst = &org_block->instructions[org_block->instructions_len - 1];

  if (last_inst->opcode == OPCODE_BR) {
    if (last_inst->operands[1].ref == i_dst_block)
      last_inst->operands[1].ref = block.index;
    else last_inst->operands[2].ref = block.index;
  } else last_inst->operands[0].ref = block.index;

  for (int i = 0; i < dst_block->prevs_len; i++) {
    if (dst_block->prevs[i]->index != i_org_block) continue;
    dst_block->prevs[i] = &func->blocks[block.index];
    break;
  }

  for (int i = 0; i < dst_block->instructions_len; i++) {
    if (dst_block->instructions[i].opcode != OPCODE_PHI) continue;
    for (int j = 0; j < dst_block->instructions[i].operands_len; j++) {
      struct IRPhi *phi = &dst_block->instructions[i].operands[j].phi;
      if (phi->block->index != i_org_block) continue;
      phi->block = &func->blocks[block.index];
    }
  }
}

void one_stage(struct IRFunction *func, struct Arena *arena) {
  int len = func->blocks_len;
  for (int i = 0; i < len; i++) {
    struct IRBasicBlock *block = &func->blocks[i];
    struct Successors sucs = get_successors(block);

    for (int j = 0; j < sucs.length; j++) {
      if (sucs.length <= 1) break;
      struct IRBasicBlock *b = &func->blocks[sucs.blocks[j]];
      if (b->prevs_len <= 1) continue;
      critic_edge(func, i, sucs.blocks[j], arena);
    }

    free(sucs.blocks);
  }
}

struct Bag {
  int64_t dst_vl;

  struct {
    int64_t vl;
    int64_t index;
  } *org;

  size_t length;
};

struct Bags {
  int64_t index;
  struct Bag *bags;
  size_t length;
};

struct TwoStage {
  struct Bags *bags;
  size_t length;
};

struct TwoStage two_stage(struct IRFunction *func, struct Arena *arena) {
  struct Bags *bags = malloc(sizeof(struct Bags));
  size_t bags_len = 0;
  size_t bags_cap = 1;

  for (int i = 0; i < func->blocks_len; i++) {
    struct IRBasicBlock *block = &func->blocks[i];
    if (block->prevs_len <= 1) continue;
    struct IRInstruction **phis = malloc(sizeof(struct IRInstruction *));
    size_t phis_len = 0;
    size_t phis_cap = 1;
    for (int j = 0; j < block->instructions_len; j++) {
      struct IRInstruction *inst = &block->instructions[j];
      if (inst->opcode != OPCODE_PHI) continue; 
      if (phis_len == phis_cap) {
        phis_cap *= 2;
        phis = realloc(phis, phis_cap * sizeof(struct IRInstruction *));
      }
      phis[phis_len++] = inst;
    }
    if (bags_len == bags_cap) {
      bags_cap *= 2;
      bags = realloc(bags, bags_cap * sizeof(struct Bags));
    }
    bags[bags_len].bags = malloc(phis_len * sizeof(struct Bag));
    bags[bags_len].length = phis_len;
    bags[bags_len].index = block->index;
    for (int j = 0; j < phis_len; j++) {
      struct Bag *bag = &bags[bags_len].bags[j];
      bag->dst_vl = phis[j]->vl;
      bag->org = malloc(phis[j]->operands_len * sizeof(*bag->org));
      bag->length = phis[j]->operands_len;
      for (int k = 0; k < phis[j]->operands_len; k++) {
        struct IRPhi *phi = &phis[j]->operands[k].phi;
        bag->org[k].vl = phi->value;
        bag->org[k].index = phi->block->index;
      }
    }
    bags_len++;
    free(phis);
  }

  return (struct TwoStage){
    bags,
    bags_len
  };
}

void addInst(struct IRBasicBlock *block, struct IRInstruction *inst, int64_t reg, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  struct IRInstruction cp = (struct IRInstruction){
    .opcode = OPCODE_CONST,
    .vl = reg,
    .operands = arena_alloc(arena, sizeof(struct IROperand)),
    .operands_len = 1
  };

  cp.operands[0] = (struct IROperand){
    .kind = Operand_Register,
    .op_type = inst->operands[0].op_type,
    .ref = inst->vl
  };

  memmove(&block->instructions[block->instructions_len], &block->instructions[block->instructions_len - 1], (block->instructions_len - (block->instructions_len - 1)) * sizeof(struct IRInstruction));
  block->instructions[block->instructions_len - 1] = cp;
  block->instructions_len++;
}

void removePhi(struct IRBasicBlock *block, int64_t index) {
  memmove(&block->instructions[index], &block->instructions[index + 1], (block->instructions_len - index - 1) * sizeof(struct IRInstruction));
  block->instructions_len--;
}

void replaceVlPhi(struct IRFunction *func, int64_t vl, int64_t reg) {
  for (int i = 0; i < func->blocks_len; i++) {
    for (int j = 0; j < func->blocks[i].instructions_len; j++) {
      for (int k = 0; k < func->blocks[i].instructions[j].operands_len; k++) {
        struct IROperand *operand = &func->blocks[i].instructions[j].operands[k];
        if (operand->kind != Operand_Register) continue;
        if (operand->ref != vl) continue;
        operand->ref = reg;
      }
    }
  }
}

void three_stage(struct TwoStage *ts, struct IRFunction *func, struct Arena *arena) {
  int oldReg = func->reg;

  for (int i = 0; i < ts->length; i++) {
    for (int j = 0; j < ts->bags[i].length; j++) {
      struct Bag *bag = &ts->bags[i].bags[j];
      int64_t reg = func->reg++;
      for (int k = 0; k < bag->length; k++) {
        struct IRInstruction *inst = getInst(func, bag->org[k].vl);
        addInst(&func->blocks[bag->org[k].index], inst, reg, arena);
      }
    }
  }

  func->reg = oldReg;
  for (int i = 0; i < ts->length; i++) {
    for (int j = 0; j < ts->bags[i].length; j++) {
      struct Bag *bag = &ts->bags[i].bags[j];
      int64_t reg = func->reg++;
      int64_t index = findInst(func, bag->dst_vl).inst_index;
      removePhi(&func->blocks[ts->bags[i].index], index);
      replaceVlPhi(func, bag->dst_vl, reg);
      free(ts->bags[i].bags[j].org);
    }

    free(ts->bags[i].bags);
  }

  free(ts->bags);
}

void ssa_destruction(struct IRFunction *func, struct Arena *arena) {
  one_stage(func, arena);
  struct TwoStage ts = two_stage(func, arena);
  three_stage(&ts, func, arena);
}
