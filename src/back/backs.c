#include <back/backs.h>
#include <back/liveness.h>
#include "back.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Back backs(struct IRModule *ir, struct Program *program, uint8_t only_asm) {
  char *folder = malloc(strlen("/tmp/wl-XXXXXX") + 1);
  strcpy(folder, "/tmp/wl-XXXXXX");
  mkdtemp(folder);
  char *lastslash = strrchr(program->filename, '/');
  lastslash++;
  char *lastdot = strrchr(lastslash, '.');
  int name_len = lastdot - lastslash;
  int len = strlen(folder) + name_len + 4;
  char *name = malloc(len);
  snprintf(name, len, "%s/%.*s.s", folder, name_len, lastslash);

  FILE *file = fopen(name, "w");
  fprintf(file, "; wlang\n");
  fprintf(file, "default rel\n");
  resolveGlobals_code(ir, file);
  fprintf(file, "section .text\n");
  fprintf(file, "  global _start\n");
  for (int i = 0; i < ir->functions_len; i++) {
    struct Live live = {0};
    liveness(&live, &ir->functions[i], program->arena);
    code(&live, &ir->functions[i], file);
  }
  fprintf(file, "_start:\n");
  fprintf(file, "  call main\n");
  fprintf(file, "  mov edi, eax\n");
  fprintf(file, "  mov eax, 0x3C\n");
  fprintf(file, "  syscall\n");
  fclose(file);

  if (only_asm) {
    free(name);
    len = name_len + 3;
    name = malloc(len);
    snprintf(name, len, "%.*s.s", name_len, lastslash);
    return (struct Back) {
      .obj_name = name,
      .folder_name = folder
    };
  }

  char *obj_name = malloc(len);
  snprintf(obj_name, len, "%s/%.*s.o", folder, name_len, lastslash);

  len = strlen("nasm -f elf64 -o") + strlen(obj_name) + strlen(name) + 3;
  char *command = malloc(len);
  snprintf(command, len, "nasm -f elf64 -o %s %s", obj_name, name);
  system(command);
  free(command);
  free(name);
  return (struct Back){
    .obj_name = obj_name,
    .folder_name = folder
  };
}
