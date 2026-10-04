#ifndef HIVE_COMPILE_H
#define HIVE_COMPILE_H

#include <stdbool.h>
#include <stddef.h>

#include "backend.h"
#include "config.h"

bool compile_to_asm(const char *src, const char *asm_path, Targets kind);
bool assemble(const char *asm_path, const char *obj_path);
bool link_objects(const char *exe, char (*objs)[PATH_MAX_LEN], size_t n);

#endif
