#pragma once

#include <utils/modules.h>
#include <front/parser.h>
#include <utils/symbols.h>

struct SymbolTable {
  struct Scope *scope;
  struct Hashmap *exports;
  struct Program *program;
  int loop;
};

void semantic(struct SymbolTable *table, struct Module *modules);
