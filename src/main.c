#include <front/parser.h>
#include <utils/error.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  struct Arena arena;
  arena.current = arenaBlock_create(4096);
  struct Args *args = resolveArgs(argc, argv, &arena);

  FILE *file = fopen(args->input_file, "r");
  if (file == NULL) errorGeneric("'%s' no such file or directory", args->input_file);

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);
  
  char *code = arena_alloc(&arena, size + 1);
  code[size] = '\0';
  fread(code, 1, size, file);
  fclose(file);

  const int CAP = 256;
  struct Tokens tokens = (struct Tokens) {
    .capacity = CAP,
    .token = calloc(1, CAP * sizeof(struct Token)),
    .length = 0
  };

  struct Program program = (struct Program) {
    .args = args,
    .arena = &arena,
    .capacity = CAP,
    .nodes = arena_alloc(&arena, CAP * sizeof(struct Node *)),
    .length = 0
  };

  lexer(args->input_file, code, &tokens);
  if (args->debugTokens) 
    showTokens(&tokens);
  
  parser(&tokens, &program);
  if (args->debugAst) 
    showAst(&program);

  free(tokens.token);
  arena_destroy(&arena);
  return 0;
}
