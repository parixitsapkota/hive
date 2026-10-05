#include "include/core/arena.h"
#include "include/syntax/lexer/location.h"

Location *mark_location(Arena *arena, size_t line, size_t column, size_t index) {
    Location *loc = arena_alloc(arena, sizeof(Location));
    *loc = (Location){line, column, index};
    return loc;
}
