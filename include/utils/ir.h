#pragma once

#include <utils/types.h>
#include <utils/literal.h>
#include <stddef.h>

#define OPCODES \
  X(OPCODE_CONST, "const") \
  X(OPCODE_ADD, "add") \
  X(OPCODE_FADD, "fadd") \
  X(OPCODE_SUB, "sub") \
  X(OPCODE_FSUB, "fsub") \
  X(OPCODE_MUL, "mul") \
  X(OPCODE_FMUL, "fmul") \
  X(OPCODE_SDIV, "sdiv") \
  X(OPCODE_UDIV, "udiv") \
  X(OPCODE_FDIV, "fdiv") \
  X(OPCODE_SREM, "srem") \
  X(OPCODE_UREM, "urem") \
  X(OPCODE_FREM, "frem") \
  X(OPCODE_BIT_AND, "and") \
  X(OPCODE_BIT_XOR, "xor") \
  X(OPCODE_BIT_OR, "or") \
  X(OPCODE_BIT_SHL, "shl") \
  X(OPCODE_BIT_ASHR, "ashr") \
  X(OPCODE_BIT_LSHR, "lshr") \
  X(OPCODE_ALLOCA, "alloca") \
  X(OPCODE_STORE, "store") \
  X(OPCODE_LOAD, "load") \
  X(OPCODE_PHI, "phi") \
  X(OPCODE_BR, "br") \
  X(OPCODE_JMP, "jmp") \
  X(OPCODE_ICMP, "icmp") \
  X(OPCODE_FCMP, "fcmp") \
  X(OPCODE_RET, "ret")

enum IRCMPKind {
  CMP_INT,
  CMP_FLOAT
};

struct IRCMP {
  enum IRCMPKind kind;

  union {
    enum IRICMP {
      ICMP_Eq,
      ICMP_Ne,
      ICMP_SGt,
      ICMP_SGe,
      ICMP_SLt,
      ICMP_SLe,
      ICMP_UGt,
      ICMP_UGe,
      ICMP_ULt,
      ICMP_ULe
    } icmp;

    enum IRFCMP {
      FCMP_OEq,
      FCMP_ONe,
      FCMP_OGt,
      FCMP_OGe,
      FCMP_OLt,
      FCMP_OLe
    } fcmp;
  };
};

struct IRPhi {
  int64_t value;
  struct IRBasicBlock *block;
};

enum IRValueKind {
  Value_Int,
  Value_Char,
  Value_UInt,
  Value_Float,
  Value_Double,
  Value_String
};

struct IRValue {
  enum IRValueKind kind;
  
  union {
    int64_t i;
    uint64_t u;
    float f;
    double d;
    struct String *s;
  };
};

enum IROperandKind {
  Operand_Global,
  Operand_Register,
  Operand_Constant,
  Operand_Block,
  Operand_Cmp,
  Operand_Phi
};

struct IROperand {
  enum IROperandKind kind;

  union {
    int64_t ref;
    struct IRValue constant;
    struct IRCMP cmp;
    struct IRPhi phi;
  };
};

enum IROpcode {
#define X(name, str) name,
  OPCODES
#undef X
};

struct IRInstruction {
  enum IROpcode opcode;
  struct Type *type;
  int64_t vl;
  struct IROperand *operands;
  size_t operands_len;
};

struct IRBasicBlock {
  int64_t index;
  struct IRInstruction *instructions;
  uint8_t term;
  size_t instructions_len;
  size_t instructions_cap;

  struct IRBasicBlock **prevs;
  size_t prevs_len;
  size_t prevs_cap;
  int64_t *reg;
};

struct IRParameter {
  struct Type *type;
  int64_t vl;
};

struct IRLoopContext {
  struct IRBasicBlock *breakBlock;
  struct IRBasicBlock *continueBlock;
  struct IRLoopContext *prev;
};

struct IRFunction {
  struct String *name;
  struct IRBasicBlock *blocks;
  size_t blocks_len;
  size_t blocks_cap;
  struct IRParameter *params;
  size_t params_len;
  int64_t reg;
  struct Hashmap *ssa;
  struct Symbol **defs;
  size_t defs_len;
  size_t defs_cap;
  struct IRLoopContext *loop;
};

struct IRGlobal {
  int64_t vl;
  struct Type *type;
  struct IROperand value;
};
