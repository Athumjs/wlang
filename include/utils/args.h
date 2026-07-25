#pragma once

#include <stdint.h>
#include <utils/arena.h>

#define FLAGS_LIST \
  X("--help", "", "Display this information", {}) \
  X("-o", "<file>", "Place the output into <file>", { \
    if (i + 1 >= argc) errorGeneric("missing filename after '-o'"); \
    args->output_file = argv[i]; \
  })

struct Args {
  char *input_file;
  char *output_file;
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena);
