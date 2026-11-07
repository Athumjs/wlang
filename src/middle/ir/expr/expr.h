#pragma once

#include "../ir.h"

// binary.c
struct IRExpr irAssign(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irLogical(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irCompare(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irBinary(struct Expr *expr, struct MIRContext *ctx);

// unary.c
struct IRExpr irCast(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irUnary(struct Expr *expr, struct MIRContext *ctx);

// postfix.c
struct IRExpr irMember(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irIndex(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irCall(struct Expr *expr, struct MIRContext *ctx);

// primary.c
struct IRExpr irStructExpr(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irArray(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irId(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irThis(struct Expr *expr, struct MIRContext *ctx);
struct IRExpr irLiteral(struct Expr *expr, struct MIRContext *ctx);
