#pragma once

#include <utils/tokens.h>
#include <utils/types.h>

struct Property {
  struct String name;
  struct Expr *expr;
  int line;
  int column;
};

struct Label {
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

struct Method {
  struct String name;
  struct Type *retType;
  struct Label *params;
  size_t params_len;
  size_t params_cap;
  struct Node *body;
  int line;
  int column;
};

struct Var {
  struct String name;
  struct Type *type;
  struct Expr *expr;
  int line;
  int column;
};

enum ExprKind {
  Expr_Assign,
  Expr_Logical,
  Expr_Compare,
  Expr_Binary,
  Expr_Cast,
  Expr_Unary,
  Expr_Member,
  Expr_Call,
  Expr_Struct,
  Expr_Array,
  Expr_Literal,
  Expr_Identifier
};

struct Expr {
  enum ExprKind kind;
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
      struct Expr *callee;
      struct Expr **args;
      size_t args_len;
      size_t args_cap;
    } expr_call;

    struct {
      struct Property *properties;
      size_t properties_len;
      size_t properties_cap;
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

    struct String expr_identifier;
  };
};

enum NodeKind {
  Node_Import,
  Node_Public,
  Node_Variable,
  Node_Function,
  Node_Condition,
  Node_LoopWhile,
  Node_LoopFor,
  Node_Return,
  Node_Continue,
  Node_Break,
  Node_Enum,
  Node_Struct,
  Node_Block,
  Node_Expr
};

struct Node {
  enum NodeKind kind;
  int line;
  int column;

  union {
    struct {
      struct Var *vars;
      size_t vars_len;
      size_t vars_cap;
      uint8_t isConst;
    } node_variable;

    struct {
      struct String name;
      struct Type *retType;
      struct Label *params;
      size_t params_len;
      size_t params_cap;
      struct Node *body;
    } node_function;

    struct {
      struct Expr *condition;
      struct Node *trueBody;
      struct Node *falseBody;
    } node_condition;

    struct {
      struct Expr *condition;
      struct Node *body;
      uint8_t doWhile;
    } node_loopWhile;

    struct {
      struct Node *init;
      struct Expr *condition;
      struct Expr *uptade;
      struct Node *body;
    } node_loopFor;

    struct {
      struct String name;
      struct Element *elements;
      size_t elements_len;
      size_t elements_cap;
    } node_enum;

    struct {
      struct String name;
      struct Label *properties;
      size_t properties_len;
      size_t properties_cap;
      struct Method *methods;
      size_t methods_len;
      size_t methods_cap;
    } node_struct;

    struct {
      struct Node **nodes;
      size_t nodes_len;
      size_t nodes_cap;
    } node_block;

    struct Expr *node_import;
    struct Node *node_public;
    struct Expr *node_return;
    struct Expr *node_expr;
  };
};
