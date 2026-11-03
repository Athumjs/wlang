#include "x86_64_linux.h"

void x86_64_linux_extension(uint64_t ext, uint8_t size, FILE *file) {
  switch (ext) {
    case 0:
      fprintf(file, "  %s\n", size == 8 ? "cqto" : "cltd");
  };
}
