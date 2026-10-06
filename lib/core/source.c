#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/core/mem.h"
#include "include/core/source.h"

Source *init_source(const char *file_path, const char *content, size_t len) {
    if (!content) return NULL;

    size_t line_count = 0;
    if (len > 0) {
        line_count = 1;
        for (size_t i = 0; i < len; ++i) {
            if (content[i] == '\n') ++line_count;
        }
    }

    Source *s = malloc(sizeof(Source));
    s->file_path = file_path;
    s->content = xstrndup(content, len);
    s->nlines = line_count;
    s->lines = malloc(sizeof(char *) * line_count);

    if (line_count > 0) {
        size_t line_idx = 0;
        s->lines[line_idx++] = s->content;
        for (size_t i = 0; i < len; ++i) {
            if (s->content[i] == '\n') {
                s->content[i] = '\0';
                if (line_idx < line_count) s->lines[line_idx++] = &s->content[i + 1];
            }
        }
    }

    return s;
}

void free_source(Source *s) {
    if (!s) return;
    free(s->content);
    free(s->lines);
    free(s);
}
