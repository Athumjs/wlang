#include <front/semantic.h>
#include "semantic.h"

void semantic(struct SymbolTable *table) {
  resolveSymbols(table);
  resolveScopes(table);
  resolveTypes(table);
}
