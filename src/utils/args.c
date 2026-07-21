#include <utils/args.h>
#include <utils/error.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena) {
  struct Args *args = arena_alloc(arena, sizeof(struct Args));
  memset(args, 0, sizeof(struct Args));

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "--help") == 0) {
        printf("Usage: wl [options] file...\n");
        printf("Options:\n");

#define X(flag, params, desc, body) \
        printf("  \033[33m%s %s\n", flag, params); \
        printf("      \033[32m%s\033[0m\n", desc);
        FLAGS_LIST
#undef X
        exit(0);
      }

#define X(flag, params, desc, body) \
      else if (strcmp(argv[i], flag) == 0) body
      FLAGS_LIST
#undef X

      else errorGeneric("missing argument to '%s'", argv[i]);
    }

    else {
      if (args->input_file != NULL) errorGeneric("more than one input file");
      args->input_file = argv[i];
    }
  }

  if (args->input_file == NULL) errorGeneric("no input file");

  return args;
}
