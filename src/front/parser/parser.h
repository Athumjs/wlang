#pragma once

#include <front/parser.h>

// parse_expr.c
struct Expr *parseExpr(int *i, struct Tokens *tokens, struct Program *program);

// parse_decl.c
struct Decl *parseDecl(int *i, struct Tokens *tokens, struct Program *program);

// parse_stmt.c
struct Stmt *parseStmt(int *i, struct Tokens *tokens, struct Program *program);

// parse_type.c
struct Type *parseType(int *i, struct Tokens *tokens, struct Program *program);

// parse_item.c
struct Item parseItem(int *i, struct Tokens *tokens, struct Program *program);

// ast.c
struct Expr *newExpr(int *i, struct Tokens *tokens, struct Program *program, enum ExprKind kind);
struct Decl *newDecl(int *i, struct Tokens *tokens, struct Program *program, enum DeclKind kind);
struct Stmt *newStmt(int *i, struct Tokens *tokens, struct Program *program, enum StmtKind kind);
enum TokenType peek(int i, struct Tokens *tokens);
#define PEEK() peek(*i, tokens)
union Literal consume(int *i, struct Tokens *tokens, struct Program *program, enum TokenType type);
#define CONSUME(t) consume(i, tokens, program, t)
