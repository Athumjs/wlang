#pragma once

#include <stdint.h>
#include <utils/arena.h>

struct Args
{
  char *input_file;
  char *output_file;

  char **obj_files;
  size_t obj_files_len;

  char **libs;
  size_t libs_len;

  char **libs_path;
  size_t libs_path_len;

  uint8_t use_libc;
  uint8_t only_asm;
  uint8_t only_obj;
  uint8_t show_mir;
  uint8_t show_lir;
  uint8_t no_entry;
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena);
