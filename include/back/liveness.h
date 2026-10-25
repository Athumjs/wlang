#pragma once

#include <middle/irgen.h>

struct Interval {
  int64_t vreg;
  int start, end;
  int phys;
  int spillSlot;
};

struct Live {
  struct Interval *intervals;
  size_t length;
};

void liveness(struct Live *live, struct IRFunction *func, struct Arena *arena);
