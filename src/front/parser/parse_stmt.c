#include "parser.h"
#include <string.h>

struct Stmt *stmtBlock(int *i, struct Tokens *tokens, struct Program *program) {
  struct Stmt *stmt = newStmt(i, tokens, program, Stmt_Block);
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    stmt->stmt_block.items_cap = 64;
    stmt->stmt_block.items = arena_alloc(program->arena, stmt->stmt_block.items_cap * sizeof(struct Item));

    while (PEEK() != TOKEN_RBRACE) {
      if (stmt->stmt_block.items_len == stmt->stmt_block.items_cap) {
        size_t oldCap = stmt->stmt_block.items_cap;
        stmt->stmt_block.items_cap *= 2;
        struct Item *temp = arena_alloc(program->arena, stmt->stmt_block.items_cap * sizeof(struct Item));
        memcpy(temp, stmt->stmt_block.items, oldCap * sizeof(struct Item));
        stmt->stmt_block.items = temp;
      }

      stmt->stmt_block.items[stmt->stmt_block.items_len++] = parseItem(i, tokens, program);
    }
  }

  CONSUME(TOKEN_RBRACE);
  return stmt;
}

struct Stmt *stmtIf(int *i, struct Tokens *tokens, struct Program *program) {
  struct Stmt *stmt = newStmt(i, tokens, program, Stmt_If);
  CONSUME(TOKEN_IF);
  CONSUME(TOKEN_LPAREN);
  stmt->stmt_if.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  stmt->stmt_if.trueBody = parseStmt(i, tokens, program);

  if (PEEK() == TOKEN_ELSE) {
    CONSUME(TOKEN_ELSE);
    stmt->stmt_if.falseBody = parseStmt(i, tokens, program);
  }

  return stmt;
}

struct Stmt *stmtWhile(int *i, struct Tokens *tokens, struct Program *program) {
  struct Stmt *stmt = newStmt(i, tokens, program, Stmt_While);

  if (PEEK() == TOKEN_DO) {
    stmt->stmt_while.doWhile = 1;
    CONSUME(TOKEN_DO);
    stmt->stmt_while.body = stmtBlock(i, tokens, program);
    CONSUME(TOKEN_WHILE);
    CONSUME(TOKEN_LPAREN);
    stmt->stmt_while.condition = parseExpr(i, tokens, program);
    CONSUME(TOKEN_RPAREN);
    CONSUME(TOKEN_SEMICOLON);
    return stmt;
  }

  CONSUME(TOKEN_WHILE);
  CONSUME(TOKEN_LPAREN);
  stmt->stmt_while.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  stmt->stmt_while.body = parseStmt(i, tokens, program);
  return stmt;
}

struct Stmt *stmtFor(int *i, struct Tokens *tokens, struct Program *program) {
  struct Stmt *stmt = newStmt(i, tokens, program, Stmt_For);
  CONSUME(TOKEN_FOR);
  CONSUME(TOKEN_LPAREN);
  stmt->stmt_for.init = parseDecl(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  stmt->stmt_for.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  stmt->stmt_for.update = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  stmt->stmt_for.body = parseStmt(i, tokens, program);
  return stmt;
}

struct Stmt *stmtReturn(int *i, struct Tokens *tokens, struct Program *program) {
  struct Stmt *stmt = newStmt(i, tokens, program, Stmt_Return);
  CONSUME(TOKEN_RETURN);
  if (PEEK() != TOKEN_SEMICOLON) stmt->stmt_return = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  return stmt;
}

struct Stmt *stmtEmpty(int *i, struct Tokens *tokens, struct Program *program, enum StmtKind kind) {
  struct Stmt *stmt = newStmt(i, tokens, program, kind);
  CONSUME(PEEK());
  return stmt;
}

struct Stmt *parseStmt(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_IF) return stmtIf(i, tokens, program);
  else if (PEEK() == TOKEN_DO || PEEK() == TOKEN_WHILE) return stmtWhile(i, tokens, program);
  else if (PEEK() == TOKEN_FOR) return stmtFor(i, tokens, program);
  else if (PEEK() == TOKEN_RETURN) return stmtReturn(i, tokens, program);
  else if (PEEK() == TOKEN_CONTINUE) return stmtEmpty(i, tokens, program, Stmt_Continue);
  else if (PEEK() == TOKEN_BREAK) return stmtEmpty(i, tokens, program, Stmt_Break);
  else if (PEEK() == TOKEN_LBRACE) return stmtBlock(i, tokens, program);
  else {
    struct Stmt *stmt = newStmt(i, tokens, program, Stmt_Expr);
    stmt->stmt_expr = parseExpr(i, tokens, program);
    CONSUME(TOKEN_SEMICOLON);
    return stmt;
  }
}
