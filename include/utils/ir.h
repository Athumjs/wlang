#pragma once

#include <utils/types.h>
#include <utils/literal.h>
#include <stddef.h>

enum ValueKind {
  Value_Int,
  Value_UInt,
  Value_Float,
  Value_Double
};

struct Value {
  enum ValueKind kind;
  
  union {
    int64_t i;
    uint64_t u;
    float f;
    double d;
  };
};

enum IROperandKind {
  Operand_Register,
  Operand_Constant,
  Operand_Block,
  Operand_Pointer,
  Operand_Type
};

struct Pointer {
  enum {
    Pointer_Global,
    Pointer_Local
  } kind; 

  union {
    struct IRGlobal *global;
    struct IRInstruction *inst;
  };
};

struct IROperand {
  enum IROperandKind kind;

  union {
    int64_t reg;
    struct Value constant;
    int64_t block;
    struct Type *type;
    struct Pointer pointer;
  };
};

enum IROpcode {
  Opcode_Add,
  Opcode_FAdd,
  Opcode_Sub,
  Opcode_FSub,
  Opcode_Mul,
  Opcode_FMul,
  Opcode_SDiv,
  Opcode_UDiv,
  Opcode_FDiv,
  Opcode_SRem,
  Opcode_URem,
  Opcode_FRem,
  Opcode_Bit_And,
  Opcode_Bit_Or,
  Opcode_Bit_Xor,
  Opcode_Bit_Shl,
  Opcode_Bit_AShr,
  Opcode_Bit_LShr,
  Opcode_Br,
  Opcode_Jmp,
  Opcode_Ret,
  Opcode_Alloca,
  Opcode_Store,
  Opcode_Load
};

struct IRInstruction {
  enum IROpcode opcode;
  int64_t result;
  struct IROperand operands[3];
  size_t operands_len;
};

struct IRBasicBlock {
  struct IRInstruction *instructions;
  uint8_t term;
  size_t instructions_len;
  size_t instructions_cap;
};

struct IRFunction {
  struct String *name;
  struct IRBasicBlock *blocks;
  size_t blocks_len;
  size_t blocks_cap;
  size_t regs_len;
};

struct IRGlobal {
  int64_t result;
  struct Type *type;
  struct IROperand value;
};
