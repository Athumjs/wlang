#include <utils/hashmap.h>
#include <string.h>

struct Hash *hash_create(char *key, void *value, struct Arena *arena) {
  struct Hash *hash = arena_alloc(arena, sizeof(struct Hash));

  hash->key = key;
  hash->value = value;
  hash->next = NULL;

  return hash;
}

struct Hashmap hashmap_create(size_t size, struct Arena *arena) {
  struct Hashmap hashmap = {0};

  hashmap.capacity = size;
  hashmap.buckets = arena_alloc(arena, size * sizeof(struct Hash *));
  for (int i = 0; i < size; i++) hashmap.buckets[i] = NULL;

  return hashmap;
}

uint32_t bucket_index(struct Hashmap *hashmap, char *key) {
  uint32_t hash = 0;
  while (*key) hash = hash * 31 + *key++;
  return hash % hashmap->capacity;
}

void hashmap_set(struct Hashmap *hashmap, char *key, void *value, struct Arena *arena) {
  uint32_t index = bucket_index(hashmap, key);
  struct Hash *hash = hashmap->buckets[index];

  while (hash != NULL) {
    if (strcmp(hash->key, key) == 0) {
      hash->value = value;
      return;
    }

    hash = hash->next;
  }

  struct Hash *newHash = hash_create(key, value, arena);
  hashmap->buckets[index] = newHash;
  hashmap->length++;
}

void *hashmap_get(struct Hashmap *hashmap, char *key, struct Arena *arena) {
  uint32_t index = bucket_index(hashmap, key);
  struct Hash *hash = hashmap->buckets[index];

  while (hash != NULL) {
    if (strcmp(hash->key, key) == 0) return hash->value;
    hash = hash->next;
  }

  return NULL;
}
