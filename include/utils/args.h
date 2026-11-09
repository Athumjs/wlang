#pragma once

#include <stdint.h>
#include <utils/arena.h>

struct Args
{
  char *input_file;
  char *output_file;
  uint8_t only_asm;
  uint8_t only_obj;
  uint8_t show_mir;
  uint8_t show_lir;
  uint8_t no_entry;
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena);
