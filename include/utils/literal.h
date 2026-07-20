#pragma once

#include <stddef.h>
#include <stdint.h>

struct String {
  const char *start;
  size_t length;
};

union Literal {
  struct String string;
  int64_t numInt;
  uint64_t numUint;
  float numFloat;
  double numDouble;
};
