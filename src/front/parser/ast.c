#include "parser.h"
#include <string.h>

struct Expr *newExpr(int *i, struct Tokens *tokens, struct Program *program, enum ExprKind kind) {
  struct Expr *expr = arena_alloc(program->arena, sizeof(struct Expr));
  memset(expr, 0, sizeof(struct Expr));
  *expr = (struct Expr) {
    .line = tokens->token[*i].line,
    .column = tokens->token[*i].column,
    .kind = kind
  };
  return expr;
}

struct Decl *newDecl(int *i, struct Tokens *tokens, struct Program *program, enum DeclKind kind) {
  struct Decl *decl = arena_alloc(program->arena, sizeof(struct Decl));
  memset(decl, 0, sizeof(struct Decl));
  *decl = (struct Decl) {
    .line = tokens->token[*i].line,
    .column = tokens->token[*i].column,
    .kind = kind
  };
  return decl;
}

struct Stmt *newStmt(int *i, struct Tokens *tokens, struct Program *program, enum StmtKind kind) {
  struct Stmt *stmt = arena_alloc(program->arena, sizeof(struct Stmt));
  memset(stmt, 0, sizeof(struct Stmt));
  *stmt = (struct Stmt) {
    .line = tokens->token[*i].line,
    .column = tokens->token[*i].column,
    .kind = kind
  };
  return stmt;
}
