#pragma once

#include <stddef.h>
#include <stdint.h>

enum LIROpcodes {
  LIR_COPY,
  LIR_ADD,
  LIR_SUB,
  LIR_MUL,
  LIR_DIV,
  LIR_LOAD,
  LIR_STORE,
  LIR_CMP,
  LIR_JMP,
  LIR_JCOND,
  LIR_CALL,
  LIR_RET,
  LIR_TARGET_SPECIFIC
};

enum LIROperandKind {
  LIR_Register,
  LIR_Immediate,
  LIR_Address,
  LIR_Label
};

struct LIROperand {
  enum LIROperandKind kind;
  uint8_t size;

  union {
    int64_t reg;
    int64_t imm;
    int64_t label;

    struct {
      int64_t base_reg;
      int64_t index_reg;
      uint8_t scale;
      int64_t disp;
    } mem;
  };
};

struct LIRInstruction {
  enum LIROpcodes opcode;
  uint64_t target_extension;
  struct LIROperand *operands;
  size_t operands_len;
  uint8_t is_unsigned;
};

struct LIRBasicBlocks {
  struct LIRInstruction *instructions;
  size_t instructions_cap;
  size_t instructions_len;
};

struct LIRFunction {
  struct String *name;
  struct LIRBasicBlocks *blocks;
  size_t blocks_len;
  size_t stack_size;
};
