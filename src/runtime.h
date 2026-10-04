#ifndef HIVE_RUNTIME_H
#define HIVE_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>

size_t runtime_count(void);
const char *runtime_name(size_t i);
bool runtime_find(const char *name, char *out, size_t out_size);

#endif
