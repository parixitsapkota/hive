#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_REGION_DEFAULT_CAPACITY
#define ARENA_REGION_DEFAULT_CAPACITY (1024)
#endif

typedef struct Arena Arena;

/// Initializes a new memory arena.
Arena *init_arena(size_t region_size);

/// Allocates memory from an arena.
void *arena_alloc(Arena *arena, size_t size);

/// Copies data into newly allocated arena memory.
void *arena_memdup(Arena *arena, const void *data, size_t size);

/// Resets an arena for reuse.
void arena_reset(Arena *arena);

/// Frees an entire arena and all of its regions.
void free_arena(Arena *arena);
