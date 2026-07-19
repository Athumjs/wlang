#include "utils/args.h"
#include <front/lexer.h>
#include <utils/error.h>
#include <stdint.h>
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

  lexer(args->input_file, code, &tokens, &arena);
  if (args->debugTokens) showTokens(&tokens);
  arena_destroy(&arena);
  return 0;
}
