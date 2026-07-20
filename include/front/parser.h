#pragma once

#include <front/lexer.h>
#include <utils/args.h>
#include <utils/nodes.h>

struct Program {
  struct Args *args;
  struct Arena *arena;
  struct Node **nodes;
  size_t length;
  size_t capacity;
};

void parser(struct Tokens *tokens, struct Program *program);
void showAst(struct Program *program);
