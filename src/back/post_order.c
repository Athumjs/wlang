#include "backs.h"
#include <stdlib.h>

void visit(struct IRBasicBlock **order, size_t *order_len, uint8_t *visited, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena) {
  visited[block->index] = 1;
  struct Successors sucs = get_successors(block);
  for (int i = 0; i < sucs.length; i++) {
    if (visited[sucs.blocks[i]]) continue;
    visit(order, order_len, visited, func, func->blocks[sucs.blocks[i]], arena);
  }
  free(sucs.blocks);
  order[(*order_len)++] = block;
}

void post_order(struct IRFunction *func, struct Arena *arena) {
  uint8_t visited[func->blocks_len];
  for (int i = 0; i < func->blocks_len; i++) visited[i] = 0;
  struct IRBasicBlock *order[func->blocks_len];
  size_t order_len = 0;
  visit(order, &order_len, visited, func, func->blocks[0], arena);
  int64_t old_to_new[func->blocks_len];
  int write_index = 0;
  for (int i = order_len - 1; i >= 0; i--) {
    int64_t index = order[i]->index;
    func->blocks[write_index] = order[i];
    func->blocks[write_index]->index = write_index;
    old_to_new[index] = write_index;
    write_index++;
  }
  for (int i = 0; i < func->blocks_len; i++) {
    struct IRBasicBlock *block = func->blocks[i];
    struct IRInstruction *inst = &block->instructions[block->instructions_len - 1];
    if (inst->opcode == OPCODE_JMP)
      inst->operands[0].ref = old_to_new[inst->operands[0].ref];
    else if (inst->opcode == OPCODE_BR) {
      inst->operands[1].ref = old_to_new[inst->operands[1].ref];
      inst->operands[2].ref = old_to_new[inst->operands[2].ref];
    }
  }
}
