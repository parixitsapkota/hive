#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "include/core/arena.h"
#include "include/core/diags.h"
#include "include/core/hasmap.h"
#include "include/core/mem.h"
#include "include/core/source.h"
#include "include/sema/sema.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/ast/visitor.h"

typedef struct Scope Scope;

struct Scope {
    HashMap *map;
    size_t depth;
    Scope *parent;
};

struct Sema {
    Source *src;
    AstNode *root;
    AstNode *curr_node;
    // var info
    Arena *var_info;
    // scope info
    Arena *scopes;
    Scope *global;
    Scope *current;
    size_t current_scope_depth;
    // Errors
    size_t errorc;
    bool has_main;
};

void scope_push(Sema *s) {
    ++s->current_scope_depth;
    Scope *scope = arena_alloc(s->scopes, sizeof(Scope));
    *scope = (Scope){0};
    scope->map = init_hash_map(16);
    scope->depth = s->current_scope_depth;
    scope->parent = s->current;
    s->current = scope;
}

void scope_pop(Sema *s) {
    Scope *scope = s->current;
    free_hash_map(scope->map);
    s->current = scope->parent;
}

VarInfo *scope_declare(Sema *s, const char *name, VarKind kind) {
    Scope *scope = s->current;
    HashMap *map = scope->map;
    VarInfo *info = var_info(s->var_info, kind);
    put_to_hash_map(map, name, info);
    return info;
}

VarInfo *scope_lookup(Sema *s, const char *name) {
    Scope *curr_scope = s->current;
    while (curr_scope != NULL) {
        if (curr_scope->map && is_in_hash_map(curr_scope->map, name)) {
            return (VarInfo *)get_from_hash_map(curr_scope->map, name);
        }
        curr_scope = curr_scope->parent;
    }
    return NULL;
}

bool is_lvalue(AstNode *e) {
    (void)e;
    return true;
}

Sema *init_sema(Source *src, AstNode *root) {
    Sema *s = (Sema *)xmalloc(sizeof(Sema));
    *s = (Sema){0};
    s->src = src;
    s->root = root;
    s->curr_node = root;
    s->var_info = init_arena(sizeof(VarInfo) * 1024);
    s->scopes = init_arena(sizeof(Scope) * 1024);
    return s;
}

void *sema_error(Sema *s, const char *fmt, ...) {
    ++s->errorc;
    va_list args;
    va_start(args, fmt);
    vdiag_err(s->src, s->curr_node->span, fmt, args);
    va_end(args);
    return NULL;
}

static void enter_function(Sema *s, AstNode *fn) {
    scope_push(s);
    for (AstNode *p = fn->as.func.params; p; p = p->next) {
        const char *name = p->as.ident.name;
        if (get_from_hash_map(s->current->map, name)) {
            sema_error(s, "duplicate parameter `%s'", name);
            return;
        }
        VarInfo *info = scope_declare(s, name, VAR_AUTO);
        p->as.ident.info = info;
    }
}

static void declare_var(Sema *s, AstNode *node) {
    const char *name = node->as.decl.name;

    if (scope_lookup(s, name)) {
        sema_error(s, "redefinition of variable `%s'", name);
        return;
    }

    VarKind kind = VAR_AUTO;
    switch (node->kind) {
    case AST_AUTO: break;
    case AST_EXTRN: kind = VAR_EXTRN; break;
    case AST_GLOBAL_DECL: kind = VAR_GLOBAL; break;
    default: break;
    }

    VarInfo *info = scope_declare(s, name, kind);
    node->as.decl.info = info;
}

static void resolve_ident(Sema *s, AstNode *node) {
    const char *name = node->as.ident.name;
    VarInfo *info = scope_lookup(s, name);
    if (!info) {
        sema_error(s, "undefined of variable `%s'", name);
        return;
    }
    node->as.ident.info = info;
}

static VisitAction sema_enter(AstNode *n, const VisitContext *ctx) {
    Sema *s = ctx->user_data;
    s->curr_node = n;

    if (ctx->role == ROLE_PARAM) return VISIT_SKIP_CHILDREN;

    switch (n->kind) {
    case AST_FUNCTION: enter_function(s, n); break;
    case AST_AUTO: declare_var(s, n); break;
    case AST_IDENT: resolve_ident(s, n); break;
    default: break;
    }

    return VISIT_CONTINUE;
}

static void sema_leave(AstNode *n, const VisitContext *ctx) {
    Sema *s = ctx->user_data;
    switch (n->kind) {
    case AST_FUNCTION: scope_pop(s); break;
    default: break;
    }
}

bool sema(Sema *s) {
    scope_push(s);
    s->global = s->current;

    AstVisitor v = {.enter = sema_enter, .leave = sema_leave};
    ast_walk(s->root, &v, s);

    scope_pop(s);
    return true;
}

void free_sema(Sema *s) {
    free_arena(s->var_info);
    free_arena(s->scopes);
    free(s);
}
