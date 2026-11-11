#include <utils/modules.h>
#include <utils/args.h>
#include <utils/error.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

void find_gcc_file(const char *filename, char *buffer, size_t size) {
  char command[512];
  snprintf(command, 512, "cc --print-file-name=%s", filename);
  FILE *file = popen(command, "r");
  if (file && fgets(buffer, size, file))
    buffer[strcspn(buffer, "\n")] = '\0';
  if (file) pclose(file);
}

char *get_runtime_path(uint8_t use_libc) {
  char *path = malloc(2048);
  if (use_libc) {
    char crt1[512], crti[512];
    find_gcc_file("crt1.o", crt1, 512);
    find_gcc_file("crti.o", crti, 512);
    snprintf(path, 2048, "%s %s ", crt1, crti);
    return path;
  }

  char exe_path[PATH_MAX];
  ssize_t l = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (l != -1) {
    exe_path[l] = '\0';
    char *dir = dirname(exe_path);
    snprintf(path, 2048, "%s/../lib/wlang/rtw.o ", dir);
    if (access(path, F_OK) == 0)
      return path;
  }
  snprintf(path, 2048, "./build/rtw.o ");
  return path;
}

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
      .folder_name = malloc(strlen("/tmp/wl-XXXXXX") + 1),
      .args = args
  };

  strcpy(modules.folder_name, "/tmp/wl-XXXXXX");
  mkdtemp(modules.folder_name);
  load_module(&modules, &arena, args->input_file, !args->no_entry);

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
    if (args->only_obj) {
      for (int i = 0; i < modules.objs_len; i++) {
        char *filename = strrchr(modules.objs[i], '/');
        filename++;
        int len = strlen("cp ") + (strlen(modules.objs[i]) * 2) + 2;
        char *command = malloc(len);
        snprintf(command, len, "cp %s %s", modules.objs[i], filename);
        system(command);
        free(modules.objs[i]);
        free(command);
      }
    } else {
      char *path = get_runtime_path(args->use_libc);
      char libc_end[1024] = "";
      if (args->use_libc) {
        char crtn[512];
        find_gcc_file("crtn.o", crtn, 512);
        snprintf(libc_end, 1024, "-lc %s --dynamic-linker /usr/lib/ld-linux-x86-64.so.2", crtn);
      }

      int len = strlen("ld -s -o ") + strlen(path) + strlen(args->output_file) + strlen(libc_end) + 3;
      for (int i = 0; i < modules.objs_len; i++)
        len += strlen(modules.objs[i]) + 1;

      for (int i = 0; i < args->obj_files_len; i++)
        len += strlen(args->obj_files[i]) + 1;

      for (int i = 0; i < args->libs_path_len; i++)
        len += strlen(args->libs_path[i]) + 3;

      for (int i = 0; i < args->libs_len; i++)
        len += strlen(args->libs[i]) + 3;

      char *command = malloc(len);
      snprintf(command, len, "ld -s -o %s %s", args->output_file, path);

      for (int i = 0; i < modules.objs_len; i++) {
        strcat(command, modules.objs[i]);
        strcat(command, " ");
        free(modules.objs[i]);
      }

      for (int i = 0; i < args->obj_files_len; i++) {
        strcat(command, args->obj_files[i]);
        strcat(command, " ");
      }

      for (int i = 0; i < args->libs_path_len; i++) {
        strcat(command, "-L");
        strcat(command, args->libs_path[i]);
        strcat(command, " ");
      }

      for (int i = 0; i < args->libs_len; i++) {
        strcat(command, "-l");
        strcat(command, args->libs[i]);
        strcat(command, " ");
      }

      strcat(command, libc_end);
      system(command);
      free(command);
      free(path);
    } 
  }

  

  int len = strlen("rm -rf ") + strlen(modules.folder_name) + 1;
  char *command = malloc(len);
  snprintf(command, len, "rm -rf %s", modules.folder_name);
  system(command);
  free(command);
  free(modules.objs);
  free(modules.folder_name);
  free(args->libs);
  free(args->libs_path);
  arena_destroy(&arena);
  return 0;
}
