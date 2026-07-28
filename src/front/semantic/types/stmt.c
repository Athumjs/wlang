#include "../semantic.h"
#include "./expr/expr.h"
#include <utils/error.h>

static void stmtIf(struct SymbolTable *table, struct Stmt *stmt) {
  typeExpr(table, stmt->stmt_if.condition);
  enterScope(table);
  typeStmt(table, stmt->stmt_if.trueBody);
  exitScope(table);
  if (stmt->stmt_if.falseBody != NULL) {
    enterScope(table);
    typeStmt(table, stmt->stmt_if.falseBody);
    exitScope(table);
  }
}

static void stmtWhile(struct SymbolTable *table, struct Stmt *stmt) {
  typeExpr(table, stmt->stmt_while.condition);
  enterScope(table);
  table->scope->onLoop = 1;
  typeStmt(table, stmt->stmt_while.body);
  exitScope(table);
}

static void stmtFor(struct SymbolTable *table, struct Stmt *stmt) {
  enterScope(table);
  table->scope->onLoop = 1;
  resolveDecl(table, stmt->stmt_for.init);
  typeExpr(table, stmt->stmt_for.condition);
  typeExpr(table, stmt->stmt_for.update);
  typeStmt(table, stmt->stmt_for.body);
  exitScope(table);
}

static void stmtReturn(struct SymbolTable *table, struct Stmt *stmt) {
  if (stmt->stmt_return == NULL) {
    table->scope->retType = arena_alloc(table->program->arena, sizeof(struct Type));
    table->scope->retType->kind = Type_Primitive;
    table->scope->retType->type_primitive.type = Primitive_Void;
  } else table->scope->retType = typeExpr(table, stmt->stmt_return);

  if (table->scope->expectType->kind == Type_Auto)
    table->scope->expectType = table->scope->retType;

  if (!cmpTT(table, table->scope->expectType, table->scope->retType) && !canImplicitConvert(table, table->scope->retType, table->scope->expectType)) {
    struct String t1 = getType(table->scope->retType, table->program->arena);
    struct String t2 = getType(table->scope->expectType, table->program->arena);
    errorLang(table->program->filename, stmt->line, stmt->column, "type '%.*s' is not assignable to type '%.*s'",
        t1.length, t1.start, t2.length, t2.start);
  }
}

static void stmtContinue(struct SymbolTable *table, struct Stmt *stmt) {
  if (!table->scope->onLoop) {
    errorLang(table->program->filename, stmt->line, stmt->column, "'continue' can only be used in loops");
  }
}

static void stmtBreak(struct SymbolTable *table, struct Stmt *stmt) {
  if (!table->scope->onLoop) {
    errorLang(table->program->filename, stmt->line, stmt->column, "'break' can only be used in loops");
  }
}

static void stmtBlock(struct SymbolTable *table, struct Stmt *stmt) {
  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    struct Item *item = &stmt->stmt_block.items[i];
    if (item->kind == Item_Decl) typeDecl(table, item->item_decl);
    else typeStmt(table, item->item_stmt);
  }
}

static void stmtExpr(struct SymbolTable *table, struct Stmt *stmt) {
  typeExpr(table, stmt->stmt_expr);
}

void typeStmt(struct SymbolTable *table, struct Stmt *stmt) {
  if (stmt->kind == Stmt_If) return stmtIf(table, stmt);
  else if (stmt->kind == Stmt_While) return stmtWhile(table, stmt);
  else if (stmt->kind == Stmt_For) return stmtFor(table, stmt);
  else if (stmt->kind == Stmt_Return) return stmtReturn(table, stmt);
  else if (stmt->kind == Stmt_Continue) return stmtContinue(table, stmt);
  else if (stmt->kind == Stmt_Break) return stmtBreak(table, stmt);
  else if (stmt->kind == Stmt_Block) return stmtBlock(table, stmt);
  else return stmtExpr(table, stmt);
}
