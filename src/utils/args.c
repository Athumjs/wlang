#include <utils/args.h>
#include <utils/error.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena)
{
  struct Args *args = arena_alloc(arena, sizeof(struct Args));
  memset(args, 0, sizeof(struct Args));

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--") == 0)
      break;

    if (argv[i][0] == '-' &&
        strcmp(argv[i], "-h") != 0 &&
        strcmp(argv[i], "--help") != 0)
    {
      errorGeneric("Invalid argument: %s", argv[i]);
    }
  }

  int current_op = 0;
  struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0} //
  };

  while ((current_op = getopt_long(argc, argv, "h", options, NULL)) != -1)
  {
    switch (current_op)
    {
    case 'h':
      printf("Usage: wl [options] file...\n");
      exit(0);
      break;

    case '?':
      errorGeneric("Invalid argument.\n");
      break;

    default:
      break;
    }
  }

  if (optind < argc)
  {
    char *file_path = argv[optind];
    args->input_file = file_path;
  }
  else
  {
    errorGeneric("No input file specified.\n");
  }

  return args;
}
