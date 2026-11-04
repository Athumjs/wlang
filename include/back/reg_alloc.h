#pragma once

#include <back/backs.h>

struct Interval {
  int64_t vreg;
  int start, end;
  int reg;
  int spillSlot;
  uint8_t size;
  uint8_t spill_force;
  uint8_t is_float;
};

struct Live {
  struct Interval *intervals;
  size_t length;
};

void liveness(struct Live *live, struct IRFunction *func, struct Arena *arena);
void register_allocation(struct Live *live, struct Arch *arch);
