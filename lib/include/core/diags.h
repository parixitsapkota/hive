#pragma once

#include <stdarg.h>

#include "include/core/location.h"
#include "include/core/source.h"

void diag_err(const Source *src, Span span, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
void vdiag_err(const Source *src, Span span, const char *fmt, va_list args);
void error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
[[noreturn]] void fatal(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
