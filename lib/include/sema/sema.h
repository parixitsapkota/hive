#pragma once

#include <stdbool.h>
#include <sys/cdefs.h>

#include "include/core/source.h"
#include "include/syntax/ast/ast.h"

typedef struct Sema Sema;

Sema *init_sema(Source *src, AstNode *root);

void *sema_error(Sema *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

bool sema(Sema *s);

void free_sema(Sema *s);
