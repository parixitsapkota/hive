#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "include/core/arena.h"
#include "include/core/hasmap.h"
#include "include/core/mem.h"

typedef struct HashEntry HashEntry;

struct HashEntry {
    char *key;       // Null-terminated key string (heap-allocated copy)
    void *value;     // Stored value
    bool in_use;     // 1 if occupied, 0 if empty
    HashEntry *next; // Next entry in this bucket's chain
};

struct HashMap {
    Arena *bucket_arena; // Arena used EXCLUSIVELY for bucket arrays
    size_t bucket_cap;   // Number of buckets
    size_t count;        // Number of key/value pairs stored
    HashEntry **buckets; // Array of bucket chain heads (allocated from arena)
};

// FNV-1a
size_t hash_string(const char *str) {
    size_t hash = (size_t)14695981039346656037ULL;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= (size_t)1099511628211ULL;
    }
    return hash;
}

static char *hs_strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *copy = (char *)xmalloc(len);
    if (!copy) {
        return NULL;
    }
    for (size_t i = 0; i < len; ++i) {
        copy[i] = str[i];
    }
    return copy;
}

HashMap *init_hash_map(size_t bucket_cap) {
    if (bucket_cap == 0) {
        bucket_cap = 1;
    }

    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    if (!map) {
        return NULL;
    }

    map->bucket_arena = init_arena(bucket_cap * sizeof(HashEntry *) + 1024);

    // Allocation ONLY for buckets comes from the Arena
    HashEntry **buckets =
        (HashEntry **)arena_alloc(map->bucket_arena, bucket_cap * sizeof(HashEntry *));
    if (!buckets) {
        free(map);
        free_arena(map->bucket_arena);
        return NULL;
    }
    memset((void *)buckets, 0, bucket_cap * sizeof(HashEntry *));

    map->bucket_cap = bucket_cap;
    map->count = 0;
    map->buckets = buckets;
    return map;
}

void resize_hash_map(HashMap *map, size_t new_bucket_cap) {
    if (!map || new_bucket_cap == 0 || new_bucket_cap == map->bucket_cap) {
        return;
    }

    // Allocate new bucket array from the arena
    HashEntry **new_buckets = (HashEntry **)arena_alloc(
        map->bucket_arena, new_bucket_cap * sizeof(HashEntry *));
    if (!new_buckets) {
        return;
    }
    memset((void *)new_buckets, 0, new_bucket_cap * sizeof(HashEntry *));

    // Re-thread existing nodes in place
    for (size_t i = 0; i < map->bucket_cap; ++i) {
        HashEntry *current = map->buckets[i];
        while (current != NULL) {
            HashEntry *next = current->next;
            size_t new_index = hash_string(current->key) % new_bucket_cap;

            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;

            current = next;
        }
    }

    // Note: Old bucket array memory is simply abandoned in the arena.
    // It will be reclaimed in bulk when arena_reset() or free_arena() is called.
    map->buckets = new_buckets;
    map->bucket_cap = new_bucket_cap;
}

void put_to_hash_map(HashMap *map, const char *key, void *value) {
    if (!map || !key) {
        return;
    }

    size_t index = hash_string(key) % map->bucket_cap;
    HashEntry *current = map->buckets[index];

    while (current != NULL) {
        if (current->in_use && strcmp(current->key, key) == 0) {
            current->value = value;
            return;
        }
        current = current->next;
    }

    // Entry nodes use standard heap allocation
    HashEntry *entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (!entry) {
        return;
    }

    entry->key = hs_strdup(key);
    entry->value = value;
    entry->in_use = 1;
    entry->next = map->buckets[index];

    map->buckets[index] = entry;
    ++map->count;

    if ((double)map->count > (double)map->bucket_cap * HM_LOAD_FACTOR) {
        resize_hash_map(map, map->bucket_cap * 2);
    }
}

void *get_from_hash_map(HashMap *map, const char *key) {
    if (!map || !key) {
        return NULL;
    }

    size_t index = hash_string(key) % map->bucket_cap;
    HashEntry *current = map->buckets[index];

    while (current != NULL) {
        if (current->in_use && strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

bool is_in_hash_map(HashMap *map, const char *key) {
    if (!map || !key) {
        return false;
    }

    size_t index = hash_string(key) % map->bucket_cap;
    HashEntry *current = map->buckets[index];

    while (current != NULL) {
        if (current->in_use && strcmp(current->key, key) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

bool del_from_hash_map(HashMap *map, const char *key) {
    if (!map || !key) {
        return false;
    }

    size_t index = hash_string(key) % map->bucket_cap;
    HashEntry *current = map->buckets[index];
    HashEntry *prev = NULL;

    while (current != NULL) {
        if (current->in_use && strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }

            free(current->key);
            free(current);
            --map->count;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void free_hash_map(HashMap *map) {
    if (!map) {
        return;
    }

    // Free all individual entry nodes and keys from standard heap
    for (size_t i = 0; i < map->bucket_cap; ++i) {
        HashEntry *current = map->buckets[i];
        while (current != NULL) {
            HashEntry *temp = current->next;
            free(current->key);
            free(current);
            current = temp;
        }
    }

    free_arena(map->bucket_arena);

    free(map);
}
