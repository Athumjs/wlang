#include "x86_64_linux.h"

char *x86_64_linux_inst_size(uint8_t size, uint8_t is_float) {
  switch (size) {
    case 1:
      return "b";
    case 2:
      return "w";
    case 4:
      if (is_float) return "ss";
      return "l";
    case 8:
      if (is_float) return "sd";
      return "q";
    default:
      return NULL;
  };
}
