#ifndef HIVE_MAIN_H
#define HIVE_MAIN_H

#include <stdbool.h>

#ifndef SHI_STRIP_PREFIX
#define SHI_STRIP_PREFIX
#endif
#include "dep/shi_flags.h"

#include "jobs.h"

typedef struct {
    bool *help;
    bool *version;
    bool *compile;
    bool *keep_temps;
    bool *dbg;
    Shi_Flag_List_Mut *names;
    char **output;
    char **target;
    char **threads;
} Options;

void usage(void);
void print_version(void);
void define_flags(Options *o);

bool options_to_spec(const Options *o, JobSpec *spec);

#endif
