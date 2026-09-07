#include "../semantic.h"
#include "./expr/expr.h"
#include <stdio.h>
#include <utils/error.h>

static struct Flow stmtIf(struct SymbolTable *table, struct Stmt *stmt) {
  typeExpr(table, stmt->stmt_if.condition);
  struct Flow trueFlow = typeStmt(table, stmt->stmt_if.trueBody);
  struct Flow falseFlow = (struct Flow){
    .next = 1
  };

  if (stmt->stmt_if.falseBody != NULL)
    falseFlow = typeStmt(table, stmt->stmt_if.falseBody);

  return (struct Flow){
    .next = trueFlow.next || falseFlow.next,
    .return_ = trueFlow.return_ || falseFlow.return_,
    .continue_ = trueFlow.continue_ || falseFlow.continue_,
    .break_ = trueFlow.break_ || falseFlow.break_,
  };
}

static struct Flow stmtWhile(struct SymbolTable *table, struct Stmt *stmt) {
  struct Type *type = typeExpr(table, stmt->stmt_while.condition);
  struct Flow flow = typeStmt(table, stmt->stmt_while.body);

  if (isBoolean(type) || isInteger(type)) {
    if (stmt->stmt_while.condition->expr_literal.literal.numInt == 0) return (struct Flow){
      .next = 1
    };

    return (struct Flow){
      .next = flow.break_,
      .return_ = flow.return_
    };
  }

  return (struct Flow){
    .next = 1,
    .return_ = flow.return_
  };
}

static struct Flow stmtFor(struct SymbolTable *table, struct Stmt *stmt) {
  resolveDecl(table, stmt->stmt_for.init);
  typeExpr(table, stmt->stmt_for.condition);
  typeExpr(table, stmt->stmt_for.update);
  return typeStmt(table, stmt->stmt_for.body);
}

static struct Flow stmtReturn(struct SymbolTable *table, struct Stmt *stmt) {
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

  return (struct Flow){
    .return_ = 1
  };
}

static struct Flow stmtContinue(struct SymbolTable *table, struct Stmt *stmt) {
  return (struct Flow){
    .continue_ = 1
  };
}

static struct Flow stmtBreak(struct SymbolTable *table, struct Stmt *stmt) {
  return (struct Flow){
    .break_ = 1
  };
}

static struct Flow stmtBlock(struct SymbolTable *table, struct Stmt *stmt) {
  table->scope = stmt->stmt_block.scope;

  struct Flow result = (struct Flow){
    .next = 1
  };

  for (int i = 0; i < stmt->stmt_block.items_len; i++) {
    struct Item *item = &stmt->stmt_block.items[i];

    if (item->kind == Item_Decl) {
      typeDecl(table, item->item_decl);
      continue;
    }

    struct Flow flow = typeStmt(table, item->item_stmt);

    result.return_ |= flow.return_;
    result.continue_ |= flow.continue_;
    result.break_ |= flow.break_;

    if (!flow.next) {
      stmt->stmt_block.items_len = i + 1;
      result.next = 0;
      break;
    }
  }

  return result;
}

static struct Flow stmtExpr(struct SymbolTable *table, struct Stmt *stmt) {
  typeExpr(table, stmt->stmt_expr);
  return (struct Flow){
    .next = 1
  };
}

struct Flow typeStmt(struct SymbolTable *table, struct Stmt *stmt) {
  if (stmt->kind == Stmt_If) return stmtIf(table, stmt);
  else if (stmt->kind == Stmt_While) return stmtWhile(table, stmt);
  else if (stmt->kind == Stmt_For) return stmtFor(table, stmt);
  else if (stmt->kind == Stmt_Return) return stmtReturn(table, stmt);
  else if (stmt->kind == Stmt_Continue) return stmtContinue(table, stmt);
  else if (stmt->kind == Stmt_Break) return stmtBreak(table, stmt);
  else if (stmt->kind == Stmt_Block) return stmtBlock(table, stmt);
  else return stmtExpr(table, stmt);
}
