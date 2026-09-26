#pragma once

#include <middle/irgen.h>

struct IRSSA {
  struct Symbol *symbol;
  int64_t block;
};

struct IRDef {
  uint8_t isParam;
  struct IRBasicBlock *block;

  union {
    struct IRInstruction *inst;
    struct IRParameter *param;
  };
};

// irglobal.c
void irGlobal(struct Decl *decl, struct IRModule *module, struct Arena *arena);

// irdecl.c
void irDecl(struct Decl *decl, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena);

// irstmt.c
struct IRBasicBlock *irStmt(struct Stmt *stmt, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena);

// irexpr.c
struct IROperand irExpr(struct Expr *expr, struct IRModule *module, struct IRFunction *func, struct IRBasicBlock *block, struct Arena *arena);

// ir.c
int64_t newBlock(struct IRFunction *func, struct Arena *arena);
void addPrevBlock(struct IRBasicBlock *prevBlock, struct IRBasicBlock *block, struct Arena *arena);
struct IRFunction *newFunc(struct String *name, size_t params_len, struct Param **params, struct IRModule *module, struct Arena *arena);
void newDef(struct Symbol *symbol, struct IRFunction *func, struct Arena *arena);
struct IRInstruction *newInst(enum IROpcode opcode, struct Type *type, uint8_t isVl, size_t operands_len, struct IRBasicBlock *block, struct Arena *arena);
void addOperandRef(uint8_t type, int64_t ref, struct IRInstruction *inst);
void addOperandConst(struct Expr *expr, struct IRInstruction *inst);
void addOperandPhi(int64_t value, struct IRBasicBlock *block, struct IRInstruction *inst);
void addOperandICmp(enum IRICMP opcode, struct IRInstruction *inst);
void addOperandFCmp(enum IRFCMP opcode, struct IRInstruction *inst);

// irutil.c
void setCurrentDef(struct Hashmap *hmap, struct String *str, uint8_t isParam, void *ptr, int64_t index, struct Arena *arena);
struct IRDef *getCurrentDef(struct Hashmap *hmap, struct String *str, struct IRBasicBlock *block, struct Arena *arena);
struct String getName(struct String *str, int block_index, struct Arena *arena);
struct IRValue irNumber(struct Expr *expr);
struct IRValue irValueGlobal(int64_t id, struct Expr *expr, struct IRModule *module, struct Arena *arena);
