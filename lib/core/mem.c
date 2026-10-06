#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/core/arena.h"

void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) {
        fputs("tinyc: out of memory\n", stderr);
        abort();
    }
    return p;
}

void *xrealloc(void *old, size_t n) {
    void *p = realloc(old, n ? n : 1);
    if (!p) {
        fputs("tinyc: out of memory\n", stderr);
        abort();
    }
    return p;
}

char *xstrndup(const char *s, size_t n) {
    char *p = xmalloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

char *xstrdup(const char *s) { return xstrndup(s, strlen(s)); }

char *substr(Arena *a, const char *source, size_t start, size_t length) {
    char *out = arena_alloc(a, length + 1);
    memcpy(out, source + start, length);
    out[length] = '\0';
    return out;
}
