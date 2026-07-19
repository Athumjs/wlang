#include <utils/args.h>
#include <utils/error.h>
#include <string.h>

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena) {
  struct Args *args = arena_alloc(arena, sizeof(struct Args));
  memset(args, 0, sizeof(struct Args));

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-dt") == 0) {
        args->debugTokens = 1;
      } else if (strcmp(argv[i], "-o") == 0) {
        if (i + 1 >= argc) errorGeneric("missing filename after '-o'");
        args->output_file = argv[i];
      } else errorGeneric("missing argument to '%s'", argv[i]);
    } else {
      if (args->input_file != NULL) errorGeneric("more than one input file");
      args->input_file = argv[i];
    }
  }

  if (args->input_file == NULL) errorGeneric("no input file");

  return args;
}
