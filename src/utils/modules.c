#include "back/backs.h"
#include <stdlib.h>
#include <utils/error.h>
#include <stdio.h>

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

  struct Tokens tokens = {0};
  tokens.capacity = 256;
  tokens.token = arena_alloc(arena, tokens.capacity * sizeof(struct Token));
  tokens.length = 0;

  struct Program program = {0};
  program.filename = path;
  program.arena = arena;
  program.capacity = 256;
  program.decls = arena_alloc(arena, program.capacity * sizeof(struct Decl *));
  program.length = 0;

  struct SymbolTable table = {0};
  table.scope = arena_alloc(arena, sizeof(struct Scope));
  table.scope->symbols = hashmap_new(arena, 128);
  table.exports = hashmap_new(arena, 8);
  table.scope->prev = NULL;
  table.scope->expectType = NULL;
  table.scope->retType = NULL;
  table.scope->currentStruct = NULL;
  table.program = &program;
  table.loop = 0;

  struct IRModule ir = {0};
  ir.globals_cap = 4;
  ir.globals = arena_alloc(arena, ir.globals_cap * sizeof(struct IRGlobal));
  ir.globals_len = 0;
  ir.functions_cap = 4;
  ir.functions = arena_alloc(arena, ir.functions_cap * sizeof(struct IRFunction));
  ir.functions_len = 0;

  lexer(path, code, &tokens, arena);
  parser(&tokens, &program);
  semantic(&table, modules);
  loweringAST(&program, &ir, modules->args->show_mir);
  char *obj_name = backs(&ir, &program, modules->folder_name, modules->args->only_asm, modules->args->show_lir);

  if (modules->objs_len == modules->objs_cap) {
    modules->objs_cap *= 2;
    modules->objs = realloc(modules->objs, modules->objs_cap * sizeof(char *));
  }

  modules->objs[modules->objs_len++] = obj_name;
  return table.exports;
}
