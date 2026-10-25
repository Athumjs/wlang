#include <back/backs.h>
#include <back/liveness.h>

void backs(struct IRModule *ir, struct Arena *arena) {
  for (int i = 0; i < ir->functions_len; i++) {
    struct Live live = {0};
    liveness(&live, &ir->functions[i], arena);
  }
}
