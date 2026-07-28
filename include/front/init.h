#pragma once

#include <utils/hashmap.h>
#include <utils/arena.h>

struct Module {
  struct Hashmap *exports;
};

void front_init(struct Arena *arena, char *path);
struct Hashmap *load_module(struct Module *modules, struct Arena *arena, char *path);
