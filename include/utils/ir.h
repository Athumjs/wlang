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

enum ICondition {
  ICond_Eq,
  ICond_Ne,
  ICond_SGt,
  ICond_SGe,
  ICond_SLt,
  ICond_SLe,
  ICond_UGt,
  ICond_UGe,
  ICond_ULt,
  ICond_ULe
};

enum FCondition {
  FCond_OEq,
  FCond_ONe,
  FCond_OGt,
  FCond_OGe,
  FCond_OLt,
  FCond_OLe
};

enum IROperandKind {
  Operand_Register,
  Operand_Constant,
  Operand_Block,
  Operand_Pointer,
  Operand_Type,
  Operand_ICond,
  Operand_FCond
};

struct Pointer {
  enum {
    Pointer_Global,
    Pointer_Local,
    Pointer_Param
  } kind; 

  union {
    struct IRGlobal *global;
    struct IRInstruction *inst;
    struct IRParameter *param;
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
    enum ICondition icond;
    enum FCondition fcond;
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
  Opcode_Icmp,
  Opcode_Fcmp,
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

struct IRParameter {
  struct Type *type;
  int64_t result;
};

struct IRFunction {
  struct String *name;
  struct IRBasicBlock *blocks;
  size_t blocks_len;
  size_t blocks_cap;
  size_t regs_len;
  struct IRParameter *params;
  size_t params_len;
};

struct IRGlobal {
  int64_t result;
  struct Type *type;
  struct IROperand value;
};
