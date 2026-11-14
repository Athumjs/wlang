#pragma once

#include <middle/irgen.h>

struct MIRContext {
  struct IRModule *module;
  struct IRFunction *func;
  struct IRBasicBlock *block;
  struct Arena *arena;
};

struct IRDef {
  uint8_t isParam;
  struct IRBasicBlock *block;

  union {
    struct IRInstruction *inst;
    struct IRParameter *param;
  };
};

struct IRExpr {
  struct IRBasicBlock *block;
  struct IRInstruction *inst;
  struct IROperand operand;
};

// ir.c
int64_t newBlock(struct IRFunction *func, struct Arena *arena);
void addPrevBlock(struct IRBasicBlock *prevBlock, struct IRBasicBlock *block, struct Arena *arena);
struct IRFunction *newFunc(struct String *name, size_t params_len, struct Param **params, struct IRModule *module, struct Arena *arena);
struct IRInstruction *newInst(enum IROpcode opcode, uint8_t isVl, size_t operands_len, struct IRBasicBlock *block, struct Arena *arena);
void addOperandRef(uint8_t type, int64_t ref, struct IRType op_type, struct IRInstruction *inst);
void addOperandGlobal(struct String *name, struct IRType type, struct IRInstruction *inst);
void addOperandPhi(int64_t value, struct IRType type, struct IRBasicBlock *block, struct IRInstruction *inst);
void addOperandICmp(enum IRICMP opcode, struct IRType type, struct IRInstruction *inst);
void addOperandFCmp(enum IRFCMP opcode, struct IRType type, struct IRInstruction *inst);
void addOperandType(struct Type *type, struct IRInstruction *inst, struct Arena *arena);

// global.c
void irGlobal(struct Decl *decl, struct IRModule *module, struct Arena *arena);

// decl.c
void irDecl(struct Decl *decl, struct MIRContext *ctx);

// stmt.c
struct IRBasicBlock *irStmt(struct Stmt *stmt, struct MIRContext *ctx);

// expr/expr.c
struct IRExpr irExpr(struct Expr *expr, struct MIRContext *ctx);

// util.c
struct IRType resolveTypeIR(struct Type *type, struct Arena *arena);
struct IRDef *setCurrentDef(struct Hashmap *hmap, struct String *str, uint8_t isParam, void *ptr, struct IRBasicBlock *block, struct Arena *arena);
struct IRDef *readVariable(struct Hashmap *hmap, struct String *str, struct IRType type, struct IRBasicBlock *block, struct Arena *arena);
void sealBlock(struct Hashmap *hmap, struct IRBasicBlock *block, struct Arena *arena);
struct String getName(struct String *str, int block_index, struct Arena *arena);
struct IRValue irNumber(struct Expr *expr);
struct IRValue irValueGlobal(struct Expr *expr, struct Arena *arena);

// print.c
void showIR(struct IRModule *ir);
