#pragma once

#include <front/parser.h>
#include <utils/symbols.h>

struct SymbolTable {
  struct Scope *scope;
  struct Program *program;
};

void semantic(struct SymbolTable *table);
