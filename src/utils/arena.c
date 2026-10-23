#include <utils/arena.h>
#include <stdlib.h>

struct ArenaBlock *arenaBlock_create(size_t size) {
  struct ArenaBlock *block = malloc(sizeof(struct ArenaBlock));

  block->memory = malloc(size);
  block->size = size;
  block->used = 0;
  block->prev = NULL;

  return block;
}

void *arena_alloc(struct Arena *arena, size_t size) {
  size_t align = _Alignof(max_align_t);
  size_t used = (arena->current->used + align - 1) & ~(align - 1);

  if (used + size > arena->current->size) {
    size_t newSize = arena->current->size * 2;
    while (newSize < size) newSize *= 2;
    struct ArenaBlock *block = arenaBlock_create(newSize);
    block->prev = arena->current;
    arena->current = block;
    used = 0;
  }

  void *ptr = arena->current->memory + used;
  arena->current->used = used + size;
  return ptr;
}

void arena_destroy(struct Arena *arena) {
  while (arena->current != NULL) {
    free(arena->current->memory);
    struct ArenaBlock *block = arena->current->prev;
    free(arena->current);
    arena->current = block;
  }
}
