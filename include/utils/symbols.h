#pragma once

#include <utils/literal.h>
#include <front/semantic.h>

struct SymbolTable;

enum SymbolKind {
  Symbol_Primitive,
  Symbol_Variable,
  Symbol_Function,
  Symbol_Enum,
  Symbol_EnumValue,
  Symbol_Struct,
  Symbol_Property,
  Symbol_Method
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
    } symbol_function;

    struct {
      struct Hashmap *items;
    } symbol_enum;

    struct {
      struct Hashmap *items;
    } symbol_struct;
  };
};

struct Scope {
  struct Hashmap *symbols;
  struct Scope *prev;
  struct Type *retType;
  struct Type *expectType;
  struct Symbol *currentStruct;
  uint8_t onLoop;
};

void enterScope(struct SymbolTable *table);
void exitScope(struct SymbolTable *table);
struct Symbol *findSymbol(struct Scope *scope, struct String *name);
void addSymbol(struct SymbolTable *table, struct Symbol *symbol);
