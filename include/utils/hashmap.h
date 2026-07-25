#pragma once

#include <utils/literal.h>
#include <stdint.h>
#include <utils/arena.h>

struct Entry {
  uint32_t hash;
  struct String key;
  void *value;
  uint32_t distance;
  uint8_t occupied;
};

struct Hashmap {
  struct Entry *buckets;
  size_t length;
  size_t capacity;
};

struct Hashmap *hashmap_new(struct Arena *arena, size_t size);
struct Entry hashmap_entry(uint32_t hash, struct String *key, void *value);
uint32_t hashmap_hash(struct String *key);
void hashmap_set(struct Hashmap *hmap, struct String *key, void *value, struct Arena *arena);
void *hashmap_get(struct Hashmap *hmap, struct String *key);
