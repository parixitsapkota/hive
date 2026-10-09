#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* : Table grows (doubles bucket_cap) once count / bucket_cap exceeds this. */
#ifndef HM_LOAD_FACTOR
#define HM_LOAD_FACTOR 0.75
#endif

typedef struct HashMap HashMap;

/// Initializes a new HashMap.
HashMap *init_hash_map(size_t bucket_cap);

/// Inserts or updates the value associated with `key'.
void put_to_hash_map(HashMap *map, const char *key, void *value);

/// Returns true if `key' exists in the map, false otherwise.
bool is_in_hash_map(HashMap *map, const char *key);

/// Returns the value associated with `key', or NULL if not found.
void *get_from_hash_map(HashMap *map, const char *key);

/// Removes `key' from the map. Returns true if it was present, flase otherwise.
bool del_from_hash_map(HashMap *map, const char *key);

/// Frees the entire hash map, including all chained entries and their keys.
void free_hash_map(HashMap *map);
