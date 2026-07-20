#pragma once

#include <utils/arena.h>
#include <utils/tokens.h>
#include <stddef.h>

struct Tokens {
  struct Token *token;
  size_t length;
  size_t capacity;
};

void lexer(const char *filename, const char *code, struct Tokens *tokens, struct Arena *arena);
void showTokens(struct Tokens *tokens);
