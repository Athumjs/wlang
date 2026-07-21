#include <utils/error.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

void errorGeneric(const char *error, ...) {
  va_list args;
  va_start(args, error);
  printf("\033[31m[ERROR]\033[0m\n ");
  vprintf(error, args);
  putchar('\n');
  va_end(args);
  exit(1);
}

int getNumCase(int num) {
  return num <= 0 ? 1 : (int)log10(num) + 1;
}

_Noreturn void errorLang(const char *filename, int line, int column, const char *error, ...) {
  va_list args;
  va_start(args, error);
  printf("\033[31m[ERROR]\033[0m %s:%d:%d\n ", filename, line, column);
  vprintf(error, args);
  putchar('\n');
  va_end(args);

  FILE *file = fopen(filename, "r");
  char buffer[1024];
  int l = 0;

  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    l++;
    if (l != line) continue;
    printf(" \033[32m%d | %s  ", line, buffer);
    for (int i = 0; i < getNumCase(line); i++) putchar(' ');
    putchar('|');
    for (int i = 0; i < column; i++) putchar(' ');
    puts("^~~\033[0m");
    break;
  }

  fclose(file);
  exit(1);
}
