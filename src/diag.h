#ifndef HIVE_DIAG_H
#define HIVE_DIAG_H

#include <stdbool.h>

void diag_init(const char *prog);
const char *diag_prog(void);

#if defined(__GNUC__)
bool diag_error(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
bool diag_error(const char *fmt, ...);
#endif

#endif
