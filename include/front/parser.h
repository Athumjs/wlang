#pragma once

#include <utils/types.h>
#include <front/lexer.h>
#include <utils/args.h>
#include <utils/nodes.h>

struct Program {
  char *filename;
  struct Arena *arena;
  struct Decl **decls;
  size_t length;
  size_t capacity;
};

void parser(struct Tokens *tokens, struct Program *program);
