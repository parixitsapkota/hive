#pragma once

#include <llvm-c/Core.h>

#include "include/core/source.h"
#include "include/syntax/ast/ast.h"

typedef struct Cgen Cgen;

Cgen *init_cgen(Source *src, AstNode *root);

LLVMModuleRef cgen(Cgen *c);

void free_cgen(Cgen *c);
