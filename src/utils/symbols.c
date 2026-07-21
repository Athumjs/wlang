#include <utils/symbols.h>
#include <utils/hashmap.h>
#include <utils/error.h>
#include <string.h>

void enterScope(struct SymbolTable *table) {
  struct Scope *temp = arena_alloc(table->program->arena, sizeof(struct Scope));
  memset(temp, 0, sizeof(struct Scope));
  temp->symbols = hashmap_new(table->program->arena, 64 * sizeof(struct Symbol *));
  temp->prev = table->scope;
  table->scope = temp;
}

void exitScope(struct SymbolTable *table) {
  table->scope = table->scope->prev;
}

struct Symbol *findSymbol(struct Scope *scope, struct String *name) {
  struct Symbol *symbol = hashmap_get(scope->symbols, name);
  if (symbol != NULL) return symbol;
  if (scope->prev == NULL) return NULL;
  return findSymbol(scope->prev, name);
}

void addSymbol(struct SymbolTable *table, struct Symbol *symbol) {
  if (findSymbol(table->scope, &symbol->name) != NULL) {
    errorLang(table->program->args->input_file, symbol->line, symbol->column, "Identifier '%.*s' has been declared", symbol->name.length, symbol->name.start);
  }

  hashmap_set(table->scope->symbols, &symbol->name, symbol, table->program->arena);
}
