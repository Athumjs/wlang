#include <front/semantic.h>
#include "semantic.h"
#include <utils/error.h>

void semantic(struct SymbolTable *table, struct Module *modules) {
  resolveSymbols(table);

  struct String str = (struct String){
    .start = "main",
    .length = 4
  };
  struct Symbol *symbol = findSymbol(table->scope, &str);

  if (symbol == NULL)
    errorGeneric("undefined reference to 'main'");

  if (symbol->type->type_function.retType->kind == Type_Auto) {
    symbol->type->type_function.retType->kind = Type_Primitive;
    symbol->type->type_function.retType->type_primitive.type = Primitive_Integer;
  }

  if (!cmpTP(symbol->type->type_function.retType, Primitive_Integer))
    errorLang(table->program->filename, symbol->line, symbol->column, "invalid type of reference 'main'");

  resolveImports(table, modules);
  resolveScopes(table);
  resolveTypes(table);
  resolveTypes(table);
}
