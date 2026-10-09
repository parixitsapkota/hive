#pragma once

#include <llvm-c/Core.h>

#include "include/core/source.h"
#include "include/syntax/ast/ast.h"

typedef struct Cgen Cgen;

Cgen *init_cgen(Source *src, AstNode *root);

void cgen(Cgen *c);

bool cgen_emit_object(Cgen *c, const char *path);

void free_cgen(Cgen *c);
