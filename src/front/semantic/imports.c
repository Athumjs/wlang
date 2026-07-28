#include "semantic.h"
#include <string.h>
#include <utils/hashmap.h>

char *resolvePath(struct Program *program, struct String import) {
  char *last = strrchr(program->filename, '/');
  int len = last - program->filename + 1;
  char *path = arena_alloc(program->arena, len + import.length + 3);
  memcpy(path, program->filename, len);
  memcpy(path + len, import.start, import.length);
  memcpy(path + len + import.length, ".w", 2);
  path[len + import.length + 2] = '\0';
  return path;
}

void resolveImport(struct SymbolTable *table, struct Module *modules, struct Decl *decl) {
  if (decl->decl_Import.local) {
    struct Module *module = hashmap_get(modules->exports, &decl->decl_Import.import_local);

    if (module == NULL) {
      char *path = resolvePath(table->program, decl->decl_Import.import_local);
      module = arena_alloc(table->program->arena, sizeof(struct Module));
      module->exports = table->exports;
      hashmap_set(modules->exports, &decl->decl_Import.import_local, module, table->program->arena);
      module->exports = load_module(modules, table->program->arena, path);
    }

    for (int i = 0; i < module->exports->capacity; i++) {
      if (!module->exports->buckets[i].occupied) continue;
      addSymbol(table, module->exports->buckets[i].value);
    }
    return;
  }
}

void resolveImports(struct SymbolTable *table, struct Module *modules) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];
    if (decl->kind != Decl_Import) continue;
    resolveImport(table, modules, decl);
  }
}
