#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/core/info.h"

void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs(BOLD FG_RED "ERROR: " RESET, stderr);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
}

[[noreturn]] void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs(BOLD FG_RED "FATAL: " RESET, stderr);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}
