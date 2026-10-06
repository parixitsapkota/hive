#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *file_path;
    char *content;
    char **lines;
    size_t nlines;
} Source;

Source *init_source(const char *file_path, const char *content, size_t len);
void free_source(Source *src);
