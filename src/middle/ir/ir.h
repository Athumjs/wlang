#pragma once

#include <middle/irgen.h>

// irgen.c
struct IROperand irOperand(struct Expr *expr, struct IRModule *ir, struct Arena *arena);
struct Value irValue(int id, struct Expr *expr, struct IRModule *ir, struct Arena *arena);

// irdecl.c
void irDecl(struct Decl *decl, struct IRModule *ir, struct Arena *arena);

// irstmt.c
void irStmt(struct Stmt *stmt, struct IRModule *ir, struct Arena *arena);

// irexpr.c
void irExpr(struct Expr *expr, struct IRModule *ir, struct IRBasicBlock *block, struct Arena *arena);
