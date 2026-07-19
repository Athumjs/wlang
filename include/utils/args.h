#pragma once

#include <stdint.h>
#include <utils/arena.h>

struct Args {
  char *input_file;
  char *output_file;
  uint8_t debugTokens;
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena);
