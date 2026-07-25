#include "semantic.h"
#include <utils/error.h>

static void stmtIf(struct SymbolTable *table, struct Stmt *stmt) {
  resolveExpr(table, stmt->stmt_if.condition);
  enterScope(table);
  resolveStmt(table, stmt->stmt_if.trueBody);
  exitScope(table);
  if (stmt->stmt_if.falseBody != NULL) {
    enterScope(table);
    resolveStmt(table, stmt->stmt_if.falseBody);
    exitScope(table);
  }
}

static void stmtWhile(struct SymbolTable *table, struct Stmt *stmt) {
  resolveExpr(table, stmt->stmt_while.condition);
  enterScope(table);
  table->scope->onLoop = 1;
  resolveStmt(table, stmt->stmt_while.body);
  exitScope(table);
}

static void stmtFor(struct SymbolTable *table, struct Stmt *stmt) {
  enterScope(table);
  table->scope->onLoop = 1;
  resolveDecl(table, stmt->stmt_for.init);
  resolveExpr(table, stmt->stmt_for.condition);
  resolveExpr(table, stmt->stmt_for.update);
  resolveStmt(table, stmt->stmt_for.body);
  exitScope(table);
}

static void stmtReturn(struct SymbolTable *table, struct Stmt *stmt) {
  if (stmt->stmt_return == NULL) {
    table->scope->retType = arena_alloc(table->program->arena, sizeof(struct Type));
    table->scope->retType->kind = Type_Primitive;
    table->scope->retType->type_primitive.type = Primitive_Void;
  } else resolveExpr(table, stmt->stmt_return);
}

static void stmtContinue(struct SymbolTable *table, struct Stmt *stmt) {
  if (!table->scope->onLoop) {
    errorLang(table->program->args->input_file, stmt->line, stmt->column, "'continue' can only be used in loops");
  }
}

static void stmtBreak(struct SymbolTable *table, struct Stmt *stmt) {
  if (!table->scope->onLoop) {
    errorLang(table->program->args->input_file, stmt->line, stmt->column, "'break' can only be used in loops");
  }
}

static void stmtBlock(struct SymbolTable *table, struct Stmt *stmt) {
  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    struct Item *item = &stmt->stmt_block.items[i];
    if (item->kind == Item_Decl) resolveDecl(table, item->item_decl);
    else resolveStmt(table, item->item_stmt);
  }
}

static void stmtExpr(struct SymbolTable *table, struct Stmt *stmt) {
  resolveExpr(table, stmt->stmt_expr);
}

void resolveStmt(struct SymbolTable *table, struct Stmt *stmt) {
  if (stmt->kind == Stmt_If) return stmtIf(table, stmt);
  else if (stmt->kind == Stmt_While) return stmtWhile(table, stmt);
  else if (stmt->kind == Stmt_For) return stmtFor(table, stmt);
  else if (stmt->kind == Stmt_Return) return stmtReturn(table, stmt);
  else if (stmt->kind == Stmt_Continue) return stmtContinue(table, stmt);
  else if (stmt->kind == Stmt_Break) return stmtBreak(table, stmt);
  else if (stmt->kind == Stmt_Block) return stmtBlock(table, stmt);
  else return stmtExpr(table, stmt);
}
