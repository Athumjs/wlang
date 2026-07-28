#pragma once

#include <front/init.h>
#include <front/parser.h>
#include <utils/symbols.h>

struct SymbolTable {
  struct Scope *scope;
  struct Hashmap *exports;
  struct Program *program;
};

void semantic(struct SymbolTable *table, struct Module *modules);
