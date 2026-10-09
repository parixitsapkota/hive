#include "include/syntax/ast/visitor.h"
#include "include/syntax/ast/ast.h"

typedef struct {
    const AstVisitor *v;
    void *user_data;
} Walker;

static VisitAction walk_one(const Walker *w, AstNode *node, AstNode *parent,
                            VisitRole role, size_t depth);

static VisitAction walk_list(const Walker *w, AstNode *head, AstNode *parent,
                             VisitRole role, size_t depth) {
    for (AstNode *n = head; n; n = n->next)
        if (walk_one(w, n, parent, role, depth) == VISIT_STOP) return VISIT_STOP;
    return VISIT_CONTINUE;
}

static VisitAction walk_children(const Walker *w, AstNode *node, size_t d) {
#define ONE(child, role)                                                                 \
    do {                                                                                 \
        if ((child) && walk_one(w, (child), node, (role), d) == VISIT_STOP)              \
            return VISIT_STOP;                                                           \
    } while (0)
#define LIST(head, role)                                                                 \
    do {                                                                                 \
        if (walk_list(w, (head), node, (role), d) == VISIT_STOP) return VISIT_STOP;      \
    } while (0)

    switch (node->kind) {
    case AST_INT:
    case AST_CHAR:
    case AST_STRING:
    case AST_IDENT:
    case AST_NULL_STMT:
    case AST_GOTO:
    case AST_LABEL:
    case AST_CONTINUE:
    case AST_BREAK: break;

    case AST_EXPR_STMT:
    case AST_RETURN: ONE(node->as.stmt.expr, ROLE_EXPR); break;

    case AST_UNARY: ONE(node->as.unary.operand, ROLE_OPERAND); break;

    case AST_BLOCK: LIST(node->as.block.stmts, ROLE_STMT); break;

    case AST_AUTO:
    case AST_EXTRN:
    case AST_GLOBAL_DECL: LIST(node->as.decl.init, ROLE_INIT); break;

    case AST_BINARY:
    case AST_ASSIGN:
        ONE(node->as.binary.lhs, ROLE_LHS);
        ONE(node->as.binary.rhs, ROLE_RHS);
        break;

    case AST_INDEX:
        ONE(node->as.index.base, ROLE_BASE);
        ONE(node->as.index.index, ROLE_INDEX);
        break;

    case AST_WHILE:
    case AST_SWITCH:
    case AST_CASE:
        ONE(node->as.ctl.expr, ROLE_COND);
        LIST(node->as.ctl.body, ROLE_BODY);
        break;

    case AST_IF:
    case AST_TERNARY:
        ONE(node->as.cond.cond, ROLE_COND);
        LIST(node->as.cond.then_b, ROLE_THEN);
        LIST(node->as.cond.else_b, ROLE_ELSE);
        break;

    case AST_CALL: LIST(node->as.call.args, ROLE_ARG); break;

    case AST_FUNCTION:
        LIST(node->as.func.params, ROLE_PARAM);
        LIST(node->as.func.body, ROLE_BODY);
        break;
    }

#undef ONE
#undef LIST
    return VISIT_CONTINUE;
}

static VisitAction walk_one(const Walker *w, AstNode *node, AstNode *parent,
                            VisitRole role, size_t depth) {
    VisitContext ctx = {
        .depth = depth, .user_data = w->user_data, .parent = parent, .role = role};

    VisitAction act = w->v->enter ? w->v->enter(node, &ctx) : VISIT_CONTINUE;
    if (act == VISIT_STOP) return VISIT_STOP;

    if (act == VISIT_CONTINUE && walk_children(w, node, depth + 1) == VISIT_STOP)
        return VISIT_STOP;

    if (w->v->leave) w->v->leave(node, &ctx);
    return VISIT_CONTINUE;
}

VisitAction ast_walk(AstNode *root, const AstVisitor *v, void *user_data) {
    Walker w = {v, user_data};
    return walk_list(&w, root, NULL, ROLE_ROOT, 0);
}

VisitAction ast_walk_node(AstNode *node, const AstVisitor *v, void *user_data) {
    Walker w = {v, user_data};
    return node ? walk_one(&w, node, NULL, ROLE_ROOT, 0) : VISIT_CONTINUE;
}

VisitAction ast_visit(AstNode *root, AstVisitorFn cb, void *user_data) {
    AstVisitor v = {.enter = cb};
    return ast_walk(root, &v, user_data);
}
