#pragma once

#include <stddef.h>

#include "include/core/arena.h"

typedef struct {
    size_t ln; // line number;
    size_t cn; // comume number;
    size_t i;  // character index;
} Location;

Location *mark_location(Arena *arena, size_t line, size_t column, size_t index);

typedef struct {
    Location *start;
    Location *end;
} Span;

static inline Span span_merge(Span a, Span b) { return (Span){a.start, b.end}; }
