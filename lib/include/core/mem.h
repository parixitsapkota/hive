#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t n);
void *xrealloc(void *old, size_t n);
char *xstrndup(const char *s, size_t n);
char *xstrdup(const char *s);

#endif
