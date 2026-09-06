#pragma once

#include <stdint.h>
#include <utils/tokens.h>
#include <utils/symbols.h>

struct Field {
  struct String name;
  int line;
  int column;

  union {
    struct Expr *expr;
    struct Type *type;
  };
};

struct Method {
  struct String name;
  struct Type *retType;
  struct Param *params;
  size_t params_len;
  size_t params_cap;
  struct Stmt *body;
  int line;
  int column;
};

struct Var {
  struct String name;
  struct Type *type;
  struct Expr *expr;
  struct Symbol *symbol;
  int line;
  int column;
};

struct Param {
  struct String name;
  struct Type *type;
  int line;
  int column;
};

struct Element {
  struct String name;
  struct Expr *expr;
  int line;
  int column;
};

enum ItemKind {
  Item_Stmt,
  Item_Decl
};

struct Item {
  enum ItemKind kind;

  union {
    struct Stmt *item_stmt;
    struct Decl *item_decl;
  };
};

enum ExprKind {
  Expr_Assign,
  Expr_Logical,
  Expr_Compare,
  Expr_Binary,
  Expr_Cast,
  Expr_Unary,
  Expr_Member,
  Expr_Index,
  Expr_Call,
  Expr_Callback,
  Expr_Struct,
  Expr_Array,
  Expr_Literal,
  Expr_This,
  Expr_Identifier
};

struct Expr {
  enum ExprKind kind;
  struct Type *type;
  int line;
  int column;

  union {
    struct {
      struct Expr *left;
      enum TokenType op;
      struct Expr *right;
    } expr_binary;

    struct {
      struct Expr *value;
      struct Type *type;
    } expr_cast;

    struct {
      struct Expr *arg;
      enum TokenType op;
      uint8_t prefix;
    } expr_unary;

    struct {
      struct Expr *obj;
      struct Expr *member;
    } expr_member;

    struct {
      struct Expr *base;
      struct Expr *index;
    } expr_index;

    struct {
      struct Expr *callee;
      struct Expr **args;
      size_t args_len;
      size_t args_cap;
    } expr_call;

    struct {
      struct Type *retType;
      struct Param *params;
      size_t params_len;
      size_t params_cap;
      struct Stmt *body;
    } expr_callback;

    struct {
      struct Field *fields;
      size_t fields_len;
      size_t fields_cap;
    } expr_struct;

    struct {
      struct Expr **exprs;
      size_t exprs_len;
      size_t exprs_cap;
    } expr_array;

    struct {
      enum TokenType kind;
      union Literal literal;
    } expr_literal;

    struct String expr_this;

    struct {
      struct String name;
      struct Symbol *symbol;
    } expr_identifier;
  };
};

enum DeclKind {
  Decl_Import,
  Decl_Public,
  Decl_Variable,
  Decl_Function,
  Decl_Enum,
  Decl_Struct
};

struct Decl {
  enum DeclKind kind;
  int line;
  int column;

  union {
    struct {
      uint8_t local;

      union {
        struct String import_local;

        struct {
          struct String *parts;
          size_t parts_len;
          size_t parts_cap;
        } import_std;
      };
    } decl_Import;

    struct {
      struct Var *vars;
      size_t vars_len;
      size_t vars_cap;
      uint8_t isConst;
    } decl_variable;

    struct {
      struct String name;
      struct Type *retType;
      struct Param *params;
      size_t params_len;
      size_t params_cap;
      struct Stmt *body;
    } decl_function;

    struct {
      struct String name;
      struct Element *elems;
      size_t elems_len;
      size_t elems_cap;
    } decl_enum;

    struct {
      struct String name;
      struct Field *fields;
      size_t fields_len;
      size_t fields_cap;
      struct Method *methods;
      size_t methods_len;
      size_t methods_cap;
    } decl_struct;

    struct Decl *decl_public;
  };
};

enum StmtKind {
  Stmt_If,
  Stmt_While,
  Stmt_For,
  Stmt_Return,
  Stmt_Continue,
  Stmt_Break,
  Stmt_Block,
  Stmt_Expr
};

struct Stmt {
  enum StmtKind kind;
  int line;
  int column;

  union {
    struct {
      struct Expr *condition;
      struct Stmt *trueBody;
      struct Stmt *falseBody;
    } stmt_if;

    struct {
      struct Expr *condition;
      struct Stmt *body;
      uint8_t doWhile;
    } stmt_while;

    struct {
      struct Decl *init;
      struct Expr *condition;
      struct Expr *update;
      struct Stmt *body;
    } stmt_for;

    struct {
      struct Item *items;
      size_t items_len;
      size_t items_cap;
      struct Scope *scope;
      struct Type *expectType;
    } stmt_block;

    struct Expr *stmt_return;
    struct Expr *stmt_expr;
  };
};
