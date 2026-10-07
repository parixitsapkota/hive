#include "include/syntax/ast/visitor.h"
#include "include/syntax/ast/ast.h"

static VisitAction traverse_internal(AstNode *node, AstVisitorFn cb, size_t depth,
                                     void *user_data) {
    if (!node) return VISIT_CONTINUE;

    VisitContext ctx = {.depth = depth, .user_data = user_data};

    VisitAction action = cb(node, &ctx);
    if (action == VISIT_STOP) return VISIT_STOP;
    if (action == VISIT_SKIP_CHILDREN) goto visit_next;

#define VISIT_CHILD(child_ptr)                                                           \
    if (traverse_internal((child_ptr), cb, depth + 1, user_data) == VISIT_STOP)          \
    return VISIT_STOP

    switch (node->kind) {
    case AST_INT:
    case AST_CHAR:
    case AST_STRING:
    case AST_IDENT:
    case AST_NULL_STMT:
    case AST_CONTINUE:
    case AST_BREAK: break;

    case AST_EXPR_STMT:
    case AST_RETURN:
    case AST_GOTO: VISIT_CHILD(node->as.stmt.expr); break;

    case AST_LABEL: VISIT_CHILD(node->as.label.stmt); break;

    case AST_UNARY: VISIT_CHILD(node->as.unary.operand); break;

    case AST_BLOCK: VISIT_CHILD(node->as.block.stmts); break;

    case AST_AUTO:
    case AST_EXTRN:
    case AST_GLOBAL_DECL: VISIT_CHILD(node->as.decl.init); break;

    case AST_BINARY:
    case AST_ASSIGN:
        VISIT_CHILD(node->as.binary.lhs);
        VISIT_CHILD(node->as.binary.rhs);
        break;

    case AST_INDEX:
        VISIT_CHILD(node->as.index.base);
        VISIT_CHILD(node->as.index.index);
        break;

    case AST_WHILE:
    case AST_SWITCH:
    case AST_CASE:
        VISIT_CHILD(node->as.ctl.expr);
        VISIT_CHILD(node->as.ctl.body);
        break;

    case AST_IF:
    case AST_TERNARY:
        VISIT_CHILD(node->as.cond.cond);
        VISIT_CHILD(node->as.cond.then_b);
        VISIT_CHILD(node->as.cond.else_b);
        break;

    case AST_CALL: VISIT_CHILD(node->as.call.args); break;

    case AST_FUNCTION:
        VISIT_CHILD(node->as.func.params);
        VISIT_CHILD(node->as.func.body);
        break;
    }

#undef VISIT_CHILD

visit_next:
    if (node->next) {
        return traverse_internal(node->next, cb, depth, user_data);
    }

    return VISIT_CONTINUE;
}

VisitAction ast_visit(AstNode *root, AstVisitorFn callback, void *user_data) {
    return traverse_internal(root, callback, 0, user_data);
}
