#include "include/core/file.h"
#include <stdio.h>
#include <stdlib.h>

FILE *openf(const char *path, uint8_t mode) {
    const char *mode_str = mode ? "rb" : "wa";
    FILE *f = fopen(path, mode_str);
    if (!f) {
        fputs("File not found!\n", stderr);
        abort();
    }
    return f;
}

char *readf(FILE *file, size_t *bytes) {
    fseek(file, 0, SEEK_END);
    const size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!content) {
        fprintf(stderr, "Failed to allocate %zu bytes.\n", file_size);
        exit(EXIT_FAILURE);
    }

    const size_t bytes_red = fread(content, 1, file_size, file);
    if (bytes_red != file_size) {
        fprintf(stderr, "FATAL : Failed to read file.\n");
        exit(EXIT_FAILURE);
    }

    content[file_size] = '\0';

    if (bytes) {
        *bytes = bytes_red;
    }

    return content;
}
