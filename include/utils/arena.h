#pragma once

#include <stddef.h>

struct ArenaBlock {
  char *memory;
  size_t size;
  size_t used;
  struct ArenaBlock *prev;
};

struct Arena {
  struct ArenaBlock *current;
};

struct ArenaBlock *arenaBlock_create(size_t size);
void *arena_alloc(struct Arena *arena, size_t size);
void arena_destroy(struct Arena *arena);

static inline size_t alignUp(size_t size) {
  return (size + 7) & ~7;
}
