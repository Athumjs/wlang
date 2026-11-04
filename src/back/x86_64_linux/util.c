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

char *x86_64_linux_conds(enum LIRCmp cmp) {
  switch (cmp) {
    case LIRCMP_Eq:
      return "e";
    case LIRCMP_Ne:
      return "ne";
    case LIRCMP_SGt:
      return "g";
    case LIRCMP_SGe:
      return "ge";
    case LIRCMP_SLt:
      return "l";
    case LIRCMP_SLe:
      return "le";
    case LIRCMP_UGt:
      return "a";
    case LIRCMP_UGe:
      return "ae";
    case LIRCMP_ULt:
      return "b";
    case LIRCMP_ULe:
      return "be";
  }
}
