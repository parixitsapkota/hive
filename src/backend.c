#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

Targets target_string_to_kind(const char *target) {
    if (!target) {
        return UNKNOWN_TARGET;
    }
    if (strcmp(target, "ir") == 0) {
        return IR_TARGET;
    }
    if (strcmp(target, "x86_64_nasm") == 0) {
        return X86_64_NASM;
    }
    return UNKNOWN_TARGET;
}

void gen_output(Ir *ir, Targets target, const char *file_path) {
    if (target == UNKNOWN_TARGET) {
        fprintf(stderr, "FATAL: unknown target (expected: ir, x86_64_nasm)\n");
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen(file_path, "w");
    if (!f) {
        fprintf(stderr, "FATAL: Failed to open file: %s\n", file_path);
        exit(EXIT_FAILURE);
    }

    switch (target) {
    case IR_TARGET: dump_ir(ir, f); break;
    case X86_64_NASM: dump_x86_64_nasm(ir, f); break;
    default: break;
    }

    fclose(f);
}
