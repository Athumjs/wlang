#include <back/reg_alloc.h>
#include <stdlib.h>

int cmp_intervals_start(const void *x, const void *y) {
  return ((struct Interval *)x)->start - ((struct Interval *)y)->start;
}

int cmp_intervals_vreg(const void *x, const void *y) {
  return ((struct Interval *)x)->vreg - ((struct Interval *)y)->vreg;
}

void actives_clean(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs) {
  for (size_t i = 0; i < *actives_len; i++) {
    if (actives[i]->end >= interval->start) continue;
    free_regs[actives[i]->reg] = 1;
    actives[i] = actives[*actives_len - 1];
    (*actives_len)--;
    i--;
  }
}

void alloc_ireg(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs, size_t regs_len) {
  if (interval->spill_force) {
    interval->reg = -1;
    return;
  }

  for (size_t i = 0; i < regs_len; i++) {
    if (!free_regs[i]) continue;
    free_regs[i] = 0;
    interval->reg = i;
    actives[(*actives_len)++] = interval;
    return;
  }

  struct Interval *spill = interval;
  int spill_index = -1;

  for (size_t i  = 0; i < *actives_len; i++) {
    if (actives[i]->end <= spill->end) continue;
    spill = actives[i];
    spill_index = i;
  }

  if (spill != interval) {
    interval->reg = spill->reg;
    spill->reg = -1;
    actives[spill_index] = interval;
  } else interval->reg = -1;
}

void alloc_freg(struct Interval **actives, size_t *actives_len, struct Interval *interval, uint8_t *free_regs, size_t iregs_len, size_t fregs_len) {
  if (interval->spill_force) {
    interval->reg = -1;
    return;
  }

  for (size_t i = iregs_len; i < iregs_len + fregs_len; i++) {
    if (!free_regs[i]) continue;
    free_regs[i] = 0;
    interval->reg = i;
    actives[(*actives_len)++] = interval;
    return;
  }

  struct Interval *spill = interval;
  int spill_index = -1;

  for (size_t i  = 0; i < *actives_len; i++) {
    if (actives[i]->end <= spill->end) continue;
    spill = actives[i];
    spill_index = i;
  }

  if (spill != interval) {
    interval->reg = spill->reg;
    spill->reg = -1;
    actives[spill_index] = interval;
  } else interval->reg = -1;
}

void register_allocation(struct Live *live, struct Arch *arch) {
  qsort(live->intervals, live->length, sizeof(struct Interval), cmp_intervals_start);

  uint8_t *free_regs = malloc((arch->iregisters_len + arch->fregisters_len) * sizeof(uint8_t));

  for (size_t i = 0; i < arch->iregisters_len + arch->fregisters_len; i++)
    free_regs[i] = !arch->registers[i].reserve;

  struct Interval **actives = calloc(arch->iregisters_len + arch->fregisters_len, sizeof(struct Interval *));
  size_t actives_len = 0;

  for (int i = 0; i < live->length; i++) {
    actives_clean(actives, &actives_len, &live->intervals[i], free_regs);

    if (live->intervals[i].is_float)
      alloc_freg(actives, &actives_len, &live->intervals[i], free_regs, arch->iregisters_len, arch->fregisters_len);
    else alloc_ireg(actives, &actives_len, &live->intervals[i], free_regs, arch->iregisters_len);
  }

  qsort(live->intervals, live->length, sizeof(struct Interval), cmp_intervals_vreg);
  free(free_regs);
  free(actives);
}
