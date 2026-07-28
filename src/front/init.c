#include <front/init.h>
#include <front/semantic.h>
#include <utils/error.h>
#include <stdio.h>

void front_init(struct Arena *arena, char *path) {
  struct Module modules = {
    .exports = hashmap_new(arena, 8)
  };

  load_module(&modules, arena, path);
}

struct Hashmap *load_module(struct Module *modules, struct Arena *arena, char *path) {
  
  FILE *file = fopen(path, "r");
  if (file == NULL) errorGeneric("'%s' no such file or directory", path);

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  char *code = arena_alloc(arena, size + 1);
  fread(code, 1, size, file);
  code[size] = '\0';
  fclose(file);

  struct Tokens tokens;
  tokens.capacity = 256;
  tokens.token = arena_alloc(arena, tokens.capacity * sizeof(struct Token));
  tokens.length = 0;

  struct Program program;
  program.filename = path;
  program.arena = arena;
  program.capacity = 256;
  program.decls = arena_alloc(arena, program.capacity * sizeof(struct Decl *));
  program.length = 0;

  struct SymbolTable table;
  table.scope = arena_alloc(arena, sizeof(struct Scope));
  table.scope->symbols = hashmap_new(arena, 128);
  table.exports = hashmap_new(arena, 8);
  table.scope->prev = NULL;
  table.scope->expectType = NULL;
  table.scope->retType = NULL;
  table.scope->currentStruct = NULL;
  table.scope->onLoop = 0;
  table.program = &program;

  lexer(path, code, &tokens, arena);
  parser(&tokens, &program);
  semantic(&table, modules);
  return table.exports;
}
