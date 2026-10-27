#include <utils/args.h>
#include <utils/error.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

enum {
  OPT_SHOW_IR = 1000
};

struct Args *resolveArgs(int argc, char **argv, struct Arena *arena)
{
  struct Args *args = arena_alloc(arena, sizeof(struct Args));
  memset(args, 0, sizeof(struct Args));

  int current_op = 0;
  struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"only-asm", no_argument, 0, 'S'},
      {"show-ir", no_argument, 0, OPT_SHOW_IR},
      {"version", no_argument, 0, 'v'},
      {"output", required_argument, 0, 'o'},
      {0, 0, 0, 0} //
  };

  while ((current_op = getopt_long(argc, argv, "hvo:S", options, NULL)) != -1)
  {
    switch (current_op)
    {
    case 'h':
      printf("Usage: wl [options] file...\n");
      printf("\t--help or -h: Lists the commands.\n");
      printf("\t--version or -v: W compiler and language version.\n");
      printf("\t--output or -o: Specifies the path to the output file.\n");
      printf("\t--only-asm or -S: Generate only the assembly code.\n");
      printf("\t--show-ir: show intermediate representation.\n");
      exit(0);
      break;
    case 'o':
      args->output_file = optarg;
      break;

    case 'S':
      args->only_asm = 1;
      break;

    case 'v':
      printf("Compiler: v%s\n", WCOMPILER_VERSION_STR);
      exit(0);
      break; 

    case OPT_SHOW_IR:
      args->show_ir = 1;
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

  if (args->output_file == NULL)
    args->output_file = "a.out";
  else if (args->only_asm)
    errorGeneric("cannot specify '-o' with '-S'.");

  return args;
}
