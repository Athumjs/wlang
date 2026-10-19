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

  int current_op = 0;
  struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"output", required_argument, 0, 'o'},
      {0, 0, 0, 0} //
  };

  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--") == 0)
      break;

    if (argv[i][0] == '-')
    {
      int option_found = 0;
      for (int j = 0; options[j].name != NULL; j++)
      {
        if ((strncmp(argv[i], "--", 2) == 0 && strcmp(argv[i] + 2, options[j].name) == 0) ||
            (options[j].val != 0 && argv[i][1] == options[j].val &&
             argv[i][2] == '\0'))
        {
          option_found = 1;
          if (options[j].has_arg == required_argument)
            i++;
          break;
        }
      }

      if (!option_found)
        errorGeneric("Invalid argument: %s", argv[i]);
    }
  }

  while ((current_op = getopt_long(argc, argv, "ho:", options, NULL)) != -1)
  {
    switch (current_op)
    {
    case 'h':
      printf("Usage: wl [options] file...\n");
      exit(0);
      break;
    case 'o':
      args->output_file = optarg;
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
