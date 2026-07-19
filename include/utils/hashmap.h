#pragma once

#include <stdint.h>
#include <utils/arena.h>

struct Hash {
  char *key;
  void *value;
  struct Hash *next;
};

struct Hashmap {
  struct Hash **buckets;
  size_t length;
  size_t capacity;
};

struct Hash *hash_create(char *key, void *value, struct Arena *arena);
struct Hashmap hashmap_create(size_t size, struct Arena *arena);
uint32_t bucket_index(struct Hashmap *hashmap, char *key);
void hashmap_set(struct Hashmap *hashmap, char *key, void *value, struct Arena *arena);
void *hashmap_get(struct Hashmap *hashmap, char *key, struct Arena *arena);
