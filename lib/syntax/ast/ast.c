#include <stddef.h>

#include "include/core/arena.h"
#include "include/syntax/ast/ast.h"

VarInfo *var_info(Arena *arena, VarKind kind) {
    VarInfo *var = arena_alloc(arena, sizeof(VarInfo));
    *var = (VarInfo){.kind = kind};
    return var;
}

AstNode *ast_new(Arena *arena, AstKind kind, Span span) {
    AstNode *node = arena_alloc(arena, sizeof(AstNode));
    *node = (AstNode){.kind = kind, .span = span};
    return node;
}

AstNode *ast_int_val(Arena *a, Span span, size_t int_val) {
    AstNode *node = ast_new(a, AST_INT, span);
    node->as.int_val = int_val;
    return node;
}

AstNode *ast_string_val(Arena *a, Span span, const char *str, size_t str_num) {
    AstNode *node = ast_new(a, AST_STRING, span);
    node->as.string.str = str;
    node->as.string.string_num = str_num;
    return node;
}

AstNode *ast_ident_val(Arena *a, Span span, const char *name) {
    AstNode *node = ast_new(a, AST_IDENT, span);
    node->as.ident.name = name;
    node->as.ident.info = NULL;
    return node;
}

AstNode *ast_unary(Arena *a, Span span, TokenKind op, bool postfix, AstNode *operand) {
    AstNode *node = ast_new(a, AST_UNARY, span);
    node->as.unary.op = op;
    node->as.unary.postfix = postfix;
    node->as.unary.operand = operand;
    return node;
}

AstNode *ast_binary(Arena *a, Span span, AstKind kind, TokenKind op, AstNode *l,
                    AstNode *r) {
    AstNode *node = ast_new(a, kind, span);
    node->as.binary.op = op;
    node->as.binary.lhs = l;
    node->as.binary.rhs = r;
    return node;
}

AstNode *ast_call(Arena *a, Span span, const char *callee, AstNode *args, size_t argc) {
    AstNode *node = ast_new(a, AST_CALL, span);
    node->as.call.callee = callee;
    node->as.call.args = args;
    node->as.call.argc = argc;
    return node;
}

AstNode *ast_stmt(Arena *a, Span span, AstKind kind, AstNode *expr) {
    AstNode *node = ast_new(a, kind, span);
    node->as.stmt.expr = expr;
    return node;
}

AstNode *ast_label(Arena *a, Span span, AstKind kind, const char *name) {
    AstNode *node = ast_new(a, kind, span);
    node->as.label = name;
    return node;
}

AstNode *ast_index(Arena *a, Span span, AstNode *left, AstNode *index) {
    AstNode *node = ast_new(a, AST_INDEX, span);
    node->as.index.base = left;
    node->as.index.index = index;
    return node;
}

AstNode *ast_cond(Arena *a, Span span, AstNode *cond, AstNode *then_b, AstNode *else_b) {
    AstNode *node = ast_new(a, AST_IF, span);
    node->as.cond.cond = cond;
    node->as.cond.then_b = then_b;
    node->as.cond.else_b = else_b;
    return node;
}

AstNode *ast_ctl(Arena *a, Span span, AstKind kind, AstNode *cond, AstNode *body) {
    AstNode *node = ast_new(a, kind, span);
    node->as.ctl.expr = cond;
    node->as.ctl.body = body;
    return node;
}

AstNode *ast_block(Arena *a, Span span, AstNode *stmts) {
    AstNode *node = ast_new(a, AST_BLOCK, span);
    node->as.block.stmts = stmts;
    return node;
}

AstNode *ast_decl(Arena *a, Span span, AstKind kind, const char *name, bool is_vec,
                  size_t size, AstNode *init) {
    AstNode *node = ast_new(a, kind, span);
    node->as.decl.name = name;
    node->as.decl.is_vec = is_vec;
    node->as.decl.size = size;
    node->as.decl.init = init;
    return node;
}

AstNode *ast_function(Arena *a, Span span, const char *name, AstNode *params,
                      size_t nparams, AstNode *body) {
    AstNode *node = ast_new(a, AST_FUNCTION, span);
    node->as.func.name = name;
    node->as.func.params = params;
    node->as.func.nparams = nparams;
    node->as.func.body = body;
    return node;
}
