#include <utils/args.h>
#include <utils/error.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

enum {
  OPT_SHOW = 1000,
  NO_ENTRY
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena)
{
  struct Args *args = arena_alloc(arena, sizeof(struct Args));
  memset(args, 0, sizeof(struct Args));

  int current_op = 0;
  struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"only-asm", no_argument, 0, 'S'},
      {"only-obj", no_argument, 0, 'c'},
      {"show", required_argument, 0, OPT_SHOW},
      {"version", no_argument, 0, 'v'},
      {"output", required_argument, 0, 'o'},
      {"no-entry", no_argument, 0, NO_ENTRY},
      {0, 0, 0, 0} //
  };

  while ((current_op = getopt_long(argc, argv, "hvo:Sc", options, NULL)) != -1)
  {
    switch (current_op)
    {
    case 'h':
      printf("Usage: wl [options] file...\n");
      printf("\t--help or -h: lists the commands.\n");
      printf("\t--version or -v: W compiler and language version.\n");
      printf("\t--output or -o: specifies the path to the output file.\n");
      printf("\t--only-asm or -S: generate only the assembly code.\n");
      printf("\t--only-obj or -c: generate only the object code.\n");
      printf("\t--show=[mir|lir]: show specific intermediate representation.\n");
      printf("\t--no-entry: tells the compiler that there is no entry function.\n");
      exit(0);
      break;
    case 'o':
      args->output_file = optarg;
      break;

    case 'S':
      args->only_asm = 1;
      break;

    case 'c':
      args->only_obj = 1;
      break;

    case 'v':
      printf("Compiler: v%s\n", WCOMPILER_VERSION_STR);
      exit(0);
      break; 

    case OPT_SHOW: {
      char *show_arg = strdup(optarg);
      char *token = strtok(show_arg, ",");
      int tokens_len = 0;

      while (token != NULL) {
        if (strcmp(token, "mir") == 0)
          args->show_mir = 1;
        else if (strcmp(token, "lir") == 0)
          args->show_lir = 1;
        else {
          free(show_arg);
          errorGeneric("invalid argument for --emit. choose 'mir' or 'lir'.");
        }

        tokens_len++;
        token = strtok(NULL, ",");
      }

      if (tokens_len == 0)
        errorGeneric("--emit requires at least one values ('mir' or 'lir').");

      free(show_arg);
      break;
    }

    case NO_ENTRY:
      args->no_entry = 1;
      break;

    case '?':
      errorGeneric("invalid argument.");
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
    errorGeneric("no input file specified.\n");
  }

  if (args->output_file == NULL)
    args->output_file = "a.out";
  else if (args->only_asm || args->only_obj)
    errorGeneric("cannot specify '-o' with '-S' or '-c'.");

  return args;
}
