#include <utils/hashmap.h>
#include <string.h>

struct Hashmap *hashmap_new(struct Arena *arena, size_t size) {
  struct Hashmap *hmap = arena_alloc(arena, sizeof(struct Hashmap));

  hmap->capacity = size;
  hmap->buckets = arena_alloc(arena, size * sizeof(struct Entry));
  hmap->length = 0;
  memset(hmap->buckets, 0, size * sizeof(struct Entry));

  return hmap;
}

struct Entry hashmap_entry(uint32_t hash, struct String *key, void *value) {
  struct Entry entry = {
    .hash = hash,
    .key = *key,
    .value = value,
    .distance = 0,
    .occupied = 1
  };
  return entry;
}

uint32_t hashmap_hash(struct String *key) {
  uint32_t hash = 2166136261u;

  for (size_t i = 0; i < key->length; i++) {
    hash ^= (uint8_t)key->start[i];
    hash *= 16777619u;
  }

  return hash;
}

void hashmap_set(struct Hashmap *hmap, struct String *key, void *value, struct Arena *arena) {
  if (hmap->length * 100 >= hmap->capacity * 75) {
    size_t oldCap = hmap->capacity;
    struct Entry *oldBuckets = hmap->buckets;
    hmap->capacity *= 2;
    hmap->buckets = arena_alloc(arena, hmap->capacity * sizeof(struct Entry));
    memset(hmap->buckets, 0, hmap->capacity * sizeof(struct Entry));
    size_t oldLen = hmap->length;
    hmap->length = 0;
    for (size_t i = 0; i < oldCap; i++) {
      if (!oldBuckets[i].occupied) continue;
      hashmap_set(hmap, &oldBuckets[i].key, oldBuckets[i].value, arena);
    }
    hmap->length = oldLen;
  }

  uint32_t hash = hashmap_hash(key);
  uint32_t index = hash & (hmap->capacity - 1);
  struct Entry entry = hashmap_entry(hash, key, value);

  while (hmap->buckets[index].occupied) {
    struct Entry *slot = &hmap->buckets[index];

    if (slot->hash == entry.hash && slot->key.length == entry.key.length &&
        memcmp(slot->key.start, entry.key.start, entry.key.length) == 0) {
      slot->value = value;
      return;
    }

    if (entry.distance > slot->distance) {
      struct Entry temp = *slot;
      hmap->buckets[index] = entry;
      entry = temp;
    }

    index = (index + 1) & (hmap->capacity - 1);
    entry.distance++;
  }

  hmap->buckets[index] = entry;
  hmap->length++;
}

void *hashmap_get(struct Hashmap *hmap, struct String *key) {
  uint32_t hash = hashmap_hash(key);
  uint32_t index = hash & (hmap->capacity - 1);
  int distance = 0;

  while (hmap->buckets[index].occupied) {
    struct Entry *slot = &hmap->buckets[index];
    if (distance > slot->distance) return NULL;

    if (slot->hash == hash && slot->key.length == key->length && memcmp(slot->key.start, key->start, key->length) == 0) {
      return slot->value;
    }

    index = (index + 1) & (hmap->capacity - 1);
    distance++;
  }

  return NULL;
}
