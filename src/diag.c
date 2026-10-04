#include <stdarg.h>
#include <stdio.h>

#include "diag.h"

static const char *g_prog = "hive";

void diag_init(const char *prog) {
    if (prog && *prog) {
        g_prog = prog;
    }
}

const char *diag_prog(void) { return g_prog; }

bool diag_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    flockfile(stderr);
    fprintf(stderr, "%s : ", g_prog);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    funlockfile(stderr);
    va_end(ap);
    return false;
}
