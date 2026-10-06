#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// mode :
///    0 : write
///  < 1 : read
FILE *openf(const char *path, uint8_t mode);

char *readf(FILE *file, size_t *bytes);
