#include "include/core/arena.h"
#include "include/core/location.h"

Location *mark_location(Arena *arena, size_t line, size_t column, size_t index,
                        const char *file_path) {
    Location *loc = arena_alloc(arena, sizeof(Location));
    *loc = (Location){line, column, index, file_path};
    return loc;
}
