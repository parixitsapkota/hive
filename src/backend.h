#ifndef HIVE_BACKEND_H
#define HIVE_BACKEND_H

#include "ir.h"

typedef enum {
    UNKNOWN_TARGET,
    X86_64_NASM,
    IR_TARGET,
} Targets;

Targets target_string_to_kind(const char *file_path);
void gen_output(Ir *ir, Targets target, const char *file_path);

void dump_ir(Ir *ir, FILE *f);
void dump_x86_64_nasm(Ir *ir, FILE *f);

#endif // HIVE_BACKEND_H
