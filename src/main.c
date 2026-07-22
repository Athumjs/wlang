#include <front/semantic.h>
#include <utils/hashmap.h>
#include <utils/error.h>
#include <stdio.h>

int main(int argc, char **argv) {
  struct Arena arena;
  arena.current = arenaBlock_create(4096);
  struct Args *args = resolveArgs(argc, argv, &arena);

  FILE *file = fopen(args->input_file, "r");
  if (file == NULL) errorGeneric("'%s' no such file or directory", args->input_file);

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  char *code = arena_alloc(&arena, size + 1);
  fread(code, 1, size, file);
  code[size] = '\0';
  fclose(file);

  struct Tokens tokens;
  tokens.capacity = 256;
  tokens.token = arena_alloc(&arena, tokens.capacity * sizeof(struct Token));
  tokens.length = 0;

  struct Program program;
  program.args = args;
  program.arena = &arena;
  program.capacity = 256;
  program.nodes = arena_alloc(&arena, program.capacity * sizeof(struct Node *));
  program.length = 0;

  struct SymbolTable table;
  table.scope = arena_alloc(&arena, sizeof(struct Scope));
  table.scope->symbols = hashmap_new(&arena, 128);
  table.scope->prev = NULL;
  table.scope->expectType = NULL;
  table.scope->retType = NULL;
  table.scope->currentStruct = NULL;
  table.scope->onLoop = 0;
  table.program = &program;

  lexer(args->input_file, code, &tokens, &arena);
  if (args->debugTokens) showTokens(&tokens);
  parser(&tokens, &program);
  if (args->debugAst) showAst(&program);
  semantic(&table);
  arena_destroy(&arena);
  return 0;
}
