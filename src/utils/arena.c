#include <utils/arena.h>
#include <stdlib.h>

struct ArenaBlock *arenaBlock_create(size_t size) {
  struct ArenaBlock *block = malloc(sizeof(struct ArenaBlock));

  size = alignUp(size);
  block->memory = malloc(size);
  block->size = size;
  block->used = 0;
  block->prev = NULL;

  return block;
}

void *arena_alloc(struct Arena *arena, size_t size) {
  size = alignUp(size);
  
  if ((arena->current->used + size) > arena->current->size) {
    size_t newSize = (arena->current->size + size) * 2; // garante que haverá espaço sempre
    struct ArenaBlock *block = arenaBlock_create(newSize);
    block->prev = arena->current;
    arena->current = block;
  }

  void *ptr = arena->current->memory + arena->current->used;
  arena->current->used += size;

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
