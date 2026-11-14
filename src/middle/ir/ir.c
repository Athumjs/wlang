#include "ir.h"
#include <string.h>

int64_t newBlock(struct IRFunction *func, struct Arena *arena) {
  if (func->blocks_len == func->blocks_cap) {
    size_t oldCap = func->blocks_cap;
    func->blocks_cap *= 2;
    struct IRBasicBlock **temp = arena_alloc(arena, func->blocks_cap * sizeof(struct IRBasicBlock *));
    memcpy(temp, func->blocks, oldCap * sizeof(struct IRBasicBlock *));
    func->blocks = temp;
  }

  func->blocks[func->blocks_len] = arena_alloc(arena, sizeof(struct IRBasicBlock)); 
  *func->blocks[func->blocks_len++] = (struct IRBasicBlock){
    .index = func->blocks_len - 1,
    .instructions_cap = 1,
    .instructions = arena_alloc(arena, sizeof(struct IRInstruction)),
    .instructions_len = 0,
    .prevs_cap = 1,
    .prevs = arena_alloc(arena, sizeof(struct IRBasicBlock *)),
    .prevs_len = 0,
    .reg = &func->reg,
    .imcomplete_phis_cap = 1,
    .imcomplete_phis = arena_alloc(arena, sizeof(struct IRImcompletePhi)),
    .imcomplete_phis_len = 0,
    .sealed = 0,
    .term = 0
  };

  return func->blocks_len - 1;
}

void addPrevBlock(struct IRBasicBlock *prevBlock, struct IRBasicBlock *block, struct Arena *arena) {
  if (block->prevs_len == block->prevs_cap) {
    size_t oldCap = block->prevs_cap;
    block->prevs_cap *= 2;
    struct IRBasicBlock **temp = arena_alloc(arena, block->prevs_cap * sizeof(struct IRBasicBlock *));
    memcpy(temp, block->prevs, oldCap * sizeof(struct IRBasicBlock *));
    block->prevs = temp;
  }

  block->prevs[block->prevs_len++] = prevBlock;
}

struct IRFunction *newFunc(struct String *name, size_t params_len, struct Param **params, struct IRModule *module, struct Arena *arena) {
  if (module->functions_len == module->functions_cap) {
    size_t oldCap = module->functions_cap;
    module->functions_cap *= 2;
    struct IRFunction *temp = arena_alloc(arena, module->functions_cap * sizeof(struct IRFunction));
    memcpy(temp, module->functions, oldCap * sizeof(struct IRFunction));
    module->functions = temp;
  }

  struct IRFunction func = (struct IRFunction){
    .name = name,
    .blocks_cap = 1,
    .blocks = arena_alloc(arena, sizeof(struct IRBasicBlock)),
    .blocks_len = 0,
    .params = arena_alloc(arena, params_len * sizeof(struct IRParameter)),
    .params_len = params_len,
    .reg = 0,
    .ssa = hashmap_new(arena, 8),
  };

  int64_t iBlock = newBlock(&func, arena);
  for (int i = 0; i < params_len; i++) {
    struct Param *param = &(*params)[i];
    func.params[i].type = resolveTypeIR(param->type, arena);
    func.params[i].vl = func.reg++;
    setCurrentDef(func.ssa, &param->name, 1, &func.params[i], func.blocks[iBlock], arena);
  }

  struct IRBasicBlock *block = func.blocks[iBlock];
  module->functions[module->functions_len++] = func;
  block->reg = &module->functions[module->functions_len - 1].reg;
  return &module->functions[module->functions_len - 1];
}

struct IRInstruction *newInst(enum IROpcode opcode, uint8_t isVl, size_t operands_len, struct IRBasicBlock *block, struct Arena *arena) {
  if (block->instructions_len == block->instructions_cap) {
    size_t oldCap = block->instructions_cap;
    block->instructions_cap *= 2;
    struct IRInstruction *temp = arena_alloc(arena, block->instructions_cap * sizeof(struct IRInstruction));
    memcpy(temp, block->instructions, oldCap * sizeof(struct IRInstruction));
    block->instructions = temp;
  }

  block->instructions[block->instructions_len++] = (struct IRInstruction){
    .opcode = opcode,
    .vl = isVl ? (*block->reg)++ : -1,
    .operands = arena_alloc(arena, operands_len * sizeof(struct IROperand)),
    .operands_len = 0
  };

  return &block->instructions[block->instructions_len - 1];
}

/**
 * type = 1 : Block | type >= 2 : Register
**/
void addOperandRef(uint8_t type, int64_t ref, struct IRType op_type, struct IRInstruction *inst) {
  struct IROperand op = (struct IROperand){
    .ref = ref,
    .op_type = op_type
  };

  if (type == 1)
    op.kind = Operand_Block;
  else op.kind = Operand_Register;

  inst->operands[inst->operands_len++] = op;
}

void addOperandGlobal(struct String *name, struct IRType type, struct IRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Global,
    .string = name,
    .op_type = type
  };
}

void addOperandPhi(int64_t value, struct IRType type, struct IRBasicBlock *block, struct IRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Phi,
    .op_type = type,
    .phi = (struct IRPhi){
      value,
      block,
    }
  };
}

void addOperandICmp(enum IRICMP opcode, struct IRType type, struct IRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Cmp,
    .op_type = type,
    .cmp = (struct IRCMP){
      .kind = CMP_INT,
      .icmp = opcode
    }
  };
}

void addOperandFCmp(enum IRFCMP opcode, struct IRType type, struct IRInstruction *inst) {
  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Cmp,
    .op_type = type,
    .cmp = (struct IRCMP){
      .kind = CMP_FLOAT,
      .fcmp = opcode
    }
  };
}

void addOperandType(struct Type *type, struct IRInstruction *inst, struct Arena *arena) {
  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Type,
    .op_type = IRTYPE_NULL,
    .type = resolveTypeIR(type, arena)
  };
}
