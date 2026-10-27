#include <utils/modules.h>
#include <utils/args.h>
#include <utils/error.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  struct Arena arena;
  arena.current = arenaBlock_create(4096);
  struct Args *args = resolveArgs(argc, argv, &arena);

  struct Module modules = {
      .exports = hashmap_new(&arena, 8),
      .objs = malloc(sizeof(char *)),
      .objs_cap = 1,
      .objs_len = 0,
      .folder_name = NULL,
      .args = args
  };

  load_module(&modules, &arena, args->input_file);

  if (args->only_asm) {
    for (int i = 0; i < modules.objs_len; i++) {
      int len = strlen("cp ") + strlen(modules.folder_name) + (strlen(modules.objs[i]) * 2) + 3;
      char *command = malloc(len);
      snprintf(command, len, "cp %s/%s %s", modules.folder_name, modules.objs[i], modules.objs[i]);
      system(command);
      free(modules.objs[i]);
      free(command);
    }
  } else {
    int len = strlen("ld -s -o ") + strlen(args->output_file) + 2;
    for (int i = 0; i < modules.objs_len; i++)
      len += strlen(modules.objs[i]) + 1;

    char *command = malloc(len);
    snprintf(command, len, "ld -s -o %s ", args->output_file);

    for (int i = 0; i < modules.objs_len; i++) {
      strcat(command, modules.objs[i]);
      strcat(command, " ");
      free(modules.objs[i]);
    }

    system(command);
    free(command); 
  }

  

  int len = strlen("rm -rf ") + strlen(modules.folder_name) + 1;
  char *command = malloc(len);
  snprintf(command, len, "rm -rf %s", modules.folder_name);
  system(command);
  free(command);
  free(modules.objs);
  free(modules.folder_name);
  arena_destroy(&arena);
  return 0;
}
