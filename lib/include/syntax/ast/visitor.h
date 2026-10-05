#pragma once

#include <stddef.h>

#include "ast.h"

typedef enum { VISIT_CONTINUE = 0, VISIT_SKIP_CHILDREN, VISIT_STOP } VisitAction;

typedef struct {
    size_t depth;
    void *user_data;
} VisitContext;

typedef VisitAction (*AstVisitorFn)(AstNode *node, const VisitContext *ctx);

VisitAction ast_visit(AstNode *root, AstVisitorFn callback, void *user_data);
