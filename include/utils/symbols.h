#pragma once

#include <utils/literal.h>
#include <front/semantic.h>

struct SymbolTable;

enum SymbolKind {
  Symbol_Primitive,
  Symbol_Variable,
  Symbol_Function,
  Symbol_Enum,
  Symbol_Struct
};

struct Symbol {
  enum SymbolKind kind;
  struct String name;
  struct Type *type;
  int line;
  int column;

  union {
    struct {
      uint8_t isConst;
    } symbol_variable;

    struct {
      struct Label *params;
      size_t params_len;
      size_t params_cap;
    } symbol_function;

    struct {
      struct Hashmap *items;
      size_t length;
      size_t capacity;
    } symbol_enum;

    struct {
      struct Hashmap *items;
      size_t length;
      size_t capacity;
    } symbol_struct;
  };
};

struct Scope {
  struct Hashmap *symbols;
  struct Scope *prev;
  struct Type *retType;
  struct Type *expectType;
  uint8_t onLoop;
};

void enterScope(struct SymbolTable *table);
void exitScope(struct SymbolTable *table);
struct Symbol *findSymbol(struct Scope *scope, struct String *name);
void addSymbol(struct SymbolTable *table, struct Symbol *symbol);
