#pragma once

#include <stddef.h>
#include <stdint.h>

#define LIROPCODES \
  X(LIR_COPY, "copy") \
  X(LIR_STORE, "store") \
  X(LIR_LOAD, "load") \
  X(LIR_ADD, "add") \
  X(LIR_SUB, "sub") \
  X(LIR_MUL, "mul") \
  X(LIR_DIV, "div") \
  X(LIR_AND, "and") \
  X(LIR_XOR, "xor") \
  X(LIR_OR, "or") \
  X(LIR_SHL, "shl") \
  X(LIR_ASR, "asr") \
  X(LIR_LSR, "lsr") \
  X(LIR_TRUNC, "trunc") \
  X(LIR_EXT, "ext") \
  X(LIR_CVT, "cvt") \
  X(LIR_CMP, "cmp") \
  X(LIR_FCMP, "fcmp") \
  X(LIR_SETCC, "setcc") \
  X(LIR_JMP, "jmp") \
  X(LIR_JCOND, "jcond") \
  X(LIR_CALL, "call") \
  X(LIR_RET, "ret") \
  X(LIR_TARGET_SPECIFIC, "text")

enum LIRCmp {
  LIRCMP_Eq,
  LIRCMP_Ne,
  LIRCMP_SGt,
  LIRCMP_SGe,
  LIRCMP_SLt,
  LIRCMP_SLe,
  LIRCMP_UGt,
  LIRCMP_UGe,
  LIRCMP_ULt,
  LIRCMP_ULe
};

enum LIROpcodes {
#define X(name, str) name,
  LIROPCODES
#undef X
};

enum LIROperandKind {
  LIR_Register,
  LIR_Immediate,
  LIR_Address,
  LIR_Label,
  LIR_Float,
  LIR_Global
};

struct LIROperand {
  enum LIROperandKind kind;
  uint8_t size;

  union {
    int64_t reg;
    int64_t imm;
    char *label;

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
  uint64_t saved_regs;
  uint8_t has_call;
  uint8_t public;
};
