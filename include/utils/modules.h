#pragma once

#include <utils/hashmap.h>
#include <utils/arena.h>

struct Module {
  struct Hashmap *exports;
  char *folder_name;
  char **objs;
  size_t objs_len;
  size_t objs_cap;
  struct Args *args;
};

struct Hashmap *load_module(struct Module *modules, struct Arena *arena, char *path);
