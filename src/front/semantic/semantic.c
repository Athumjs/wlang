#include <front/semantic.h>
#include "semantic.h"

void semantic(struct SymbolTable *table, struct Module *modules) {
  resolveSymbols(table);
  resolveImports(table, modules);
  resolveScopes(table);
  resolveTypes(table);
}
