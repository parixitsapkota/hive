#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/core/diags.h"
#include "include/core/info.h"

static void diag_render_line(const Source *src, Span span) {

    size_t line = span.start->ln;

    if (line > src->nlines || line == 0) {
        fatal("incorrect line number: %zu for src: %s", line, src->file_path);
    }

    fprintf(stderr, " %5zu | %-72s\n", line, src->lines[line - 1]);
    fprintf(stderr, "       |");
    for (size_t i = 0; i < span.start->cn; ++i)
        fputc(' ', stderr);
    size_t cn_len = span.end->cn - span.start->cn - 1;
    fprintf(stderr, RESET FGB_GREEN);
    fputc('^', stderr);
    for (size_t i = 0; i < cn_len; ++i)
        fputc('~', stderr);
    fprintf(stderr, RESET "\n");
}

void diag_err(const Source *src, Span span, const char *fmt, ...) {
    fprintf(stderr, BOLD FGB_WHITE "%s:%zu:%zu: " RESET, src->file_path, span.start->ln,
            span.start->cn);

    va_list args;
    va_start(args, fmt);
    error(fmt, args);
    va_end(args);

    diag_render_line(src, span);
}

void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs(BOLD FGB_RED "ERROR: " FGB_WHITE, stderr);
    vfprintf(stderr, fmt, args);
    fputs(RESET "\n", stderr);
    va_end(args);
}

[[noreturn]] void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fputs(BOLD FGB_RED "FATAL: " RESET FGB_WHITE, stderr);
    vfprintf(stderr, fmt, args);
    fputs(RESET "\n", stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}
