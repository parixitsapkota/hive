#include <stdio.h>
#include <stdlib.h>

#include "include/core/diags.h"
#include "include/core/file.h"

FILE *openf(const char *path, uint8_t mode) {
    const char *mode_str = mode ? "rb" : "wa";
    FILE *f = fopen(path, mode_str);
    if (!f) fatal(" File not found! `%s`", path);
    return f;
}

char *readf(FILE *file, size_t *bytes) {
    fseek(file, 0, SEEK_END);
    const size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!content) fatal("Failed to allocate %zu bytes.", file_size + 1);
    const size_t bytes_red = fread(content, 1, file_size, file);
    if (bytes_red != file_size) fatal("Failed to read file.");
    content[file_size] = '\0';

    if (bytes) {
        *bytes = bytes_red;
    }

    return content;
}
