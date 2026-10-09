#pragma once

#include <stddef.h>

#include "ast.h"

typedef enum { VISIT_CONTINUE = 0, VISIT_SKIP_CHILDREN, VISIT_STOP } VisitAction;

typedef enum {
    ROLE_ROOT,
    ROLE_OPERAND,
    ROLE_LHS,
    ROLE_RHS,
    ROLE_BASE,
    ROLE_INDEX,
    ROLE_COND,
    ROLE_THEN,
    ROLE_ELSE,
    ROLE_BODY,
    ROLE_STMT,
    ROLE_EXPR,
    ROLE_ARG,
    ROLE_INIT,
    ROLE_PARAM,
} VisitRole;

typedef struct {
    size_t depth;
    void *user_data;
    AstNode *parent;
    VisitRole role;
} VisitContext;

typedef VisitAction (*AstVisitorFn)(AstNode *node, const VisitContext *ctx);
typedef void (*AstLeaveFn)(AstNode *node, const VisitContext *ctx);

typedef struct {
    AstVisitorFn enter;
    AstLeaveFn leave;
} AstVisitor;

VisitAction ast_walk(AstNode *root, const AstVisitor *v, void *user_data);
VisitAction ast_walk_node(AstNode *node, const AstVisitor *v, void *user_data);
VisitAction ast_visit(AstNode *root, AstVisitorFn callback, void *user_data);
