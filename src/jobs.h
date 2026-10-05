#ifndef HIVE_JOBS_H
#define HIVE_JOBS_H

#include <stdbool.h>
#include <stddef.h>

#include "backend.h"

typedef struct {
    const char *const *inputs;
    size_t n_inputs;
    bool compile_only;
    bool keep_temps;
    bool dbg;
    bool combine_o;
    const char *output;
    Targets kind;
    size_t threads;
} JobSpec;

int jobs_run(const JobSpec *spec);

#endif
