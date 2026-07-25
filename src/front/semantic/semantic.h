#pragma once

#include <front/semantic.h>

// expr/expr.c
struct Symbol *resolveExpr(struct SymbolTable *table, struct Expr *expr);

// expr/binary.c
struct Symbol *resolveExprAssign(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprLogical(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprCompare(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprBinary(struct SymbolTable *table, struct Expr *expr);

// expr/unary.c
struct Symbol *resolveExprCast(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprUnary(struct SymbolTable *table, struct Expr *expr);

// expr/postfix.c
struct Symbol *resolveExprCall(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprMember(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprIndex(struct SymbolTable *table, struct Expr *expr);

// expr/primary.c
struct Symbol *resolveExprStruct(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprArray(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprThis(struct SymbolTable *table, struct Expr *expr);
struct Symbol *resolveExprIdentifier(struct SymbolTable *table, struct Expr *expr);

// symbols.c
void addSymbolVar(struct SymbolTable *table, struct Var *var, uint8_t isConst);
void resolveSymbols(struct SymbolTable *table);
void resolveDecl(struct SymbolTable *table, struct Decl *decl);

// stmt.c
void resolveStmt(struct SymbolTable *table, struct Stmt *stmt);

// scopes.c
void resolveScopes(struct SymbolTable *table);

// types/stmt.c
void typeStmt(struct SymbolTable *table, struct Stmt *stmt);

// types/decl.c
void typeDecl(struct SymbolTable *table, struct Decl *decl);

// types/types.c
void resolveTypes(struct SymbolTable *table);

// types/expr/binary.c
struct Type *typeExprAssign(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprLogical(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprCompare(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprBinary(struct SymbolTable *table, struct Expr *expr);

// types/expr/unary.c
struct Type *typeExprCast(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprUnary(struct SymbolTable *table, struct Expr *expr);

// types/expr/postfix.c
struct Type *typeExprCall(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprMember(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprIndex(struct SymbolTable *table, struct Expr *expr);

// types/expr/primary.c
struct Type *typeExprStruct(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprArray(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprLiteral(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprThis(struct SymbolTable *table, struct Expr *expr);
struct Type *typeExprIdentifier(struct SymbolTable *table, struct Expr *expr);

// types/expr/expr.c
struct Type *typeExpr(struct SymbolTable *table, struct Expr *expr);
