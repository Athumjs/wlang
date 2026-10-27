#pragma once

#include <stdint.h>
#include <utils/arena.h>

struct Args
{
  char *input_file;
  char *output_file;
  uint8_t only_asm;
  uint8_t show_ir;
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena);
