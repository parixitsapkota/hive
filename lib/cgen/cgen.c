#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>

#include "include/cgen/cgen.h"
#include "include/core/mem.h"
#include "include/core/source.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/ast/visitor.h"

#define CGEN_STACK_INIT 64

struct Cgen {
    Source *src;
    AstNode *root;
    // llvm references
    LLVMContextRef ctx;
    LLVMModuleRef mod;
    LLVMBuilderRef b;
    LLVMTypeRef word;
    // value stack
    LLVMValueRef *stack;
    size_t sp;
    size_t cap;
    // current state
    LLVMValueRef last_val;
    LLVMValueRef curr_func;
    char curr_block_name[32];
    LLVMBasicBlockRef curr_block;
};

Cgen *init_cgen(Source *src, AstNode *root) {
    Cgen *c = xmalloc(sizeof(Cgen));
    *c = (Cgen){0};
    c->src = src;
    c->root = root;

    c->cap = CGEN_STACK_INIT;
    c->sp = 0;
    c->stack = (LLVMValueRef *)xmalloc(c->cap * sizeof(LLVMValueRef));

    c->ctx = LLVMContextCreate();
    c->mod = LLVMModuleCreateWithNameInContext(src->file_path, c->ctx);
    c->b = LLVMCreateBuilderInContext(c->ctx);
    c->word = LLVMInt64TypeInContext(c->ctx);
    return c;
}

static void push_val_ref(Cgen *c, LLVMValueRef ref) {
    if (c->sp == c->cap) {
        size_t new_cap = c->cap * 2;
        LLVMValueRef *p =
            (LLVMValueRef *)xrealloc((void *)c->stack, new_cap * sizeof(LLVMValueRef));
        if (!p) {
            fprintf(stderr, "cgen: out of memory\n");
            exit(1);
        }
        c->stack = p;
        c->cap = new_cap;
    }
    c->stack[c->sp++] = ref;
    c->last_val = ref;
}

static LLVMValueRef pop_val_ref(Cgen *c) {
    assert(c->sp > 0 && "cgen: value stack underflow");
    LLVMValueRef v = c->stack[--c->sp];
    c->last_val = c->sp ? c->stack[c->sp - 1] : NULL;
    return v;
}

static void cgen_function(Cgen *c, AstNode *fn) {
    LLVMTypeRef func_type = LLVMFunctionType(c->word, NULL, 0, 0);
    LLVMValueRef func = LLVMAddFunction(c->mod, fn->as.func.name, func_type);

    c->curr_func = func;
    c->curr_block = LLVMAppendBasicBlockInContext(c->ctx, func, "entry");
    LLVMPositionBuilderAtEnd(c->b, c->curr_block);
}

static void cgen_binary(Cgen *c, AstNode *node) {
    LLVMValueRef rhs = pop_val_ref(c);
    LLVMValueRef lhs = pop_val_ref(c);
    LLVMValueRef r = NULL;
    switch (node->as.binary.op) {
    case TOK_ADD: r = LLVMBuildAdd(c->b, lhs, rhs, "add"); break;
    case TOK_SUB: r = LLVMBuildSub(c->b, lhs, rhs, "sub"); break;
    case TOK_MUL: r = LLVMBuildMul(c->b, lhs, rhs, "mul"); break;
    default: break;
    }
    assert(r && "cgen: unsupported binary operator");
    push_val_ref(c, r);
}

static void cgen_return(Cgen *c, AstNode *node) {
    LLVMValueRef v = node->as.stmt.expr ? pop_val_ref(c) : LLVMConstInt(c->word, 0, 0);
    LLVMBuildRet(c->b, v);
}

static VisitAction cgen_enter(AstNode *node, const VisitContext *ctx) {
    Cgen *c = ctx->user_data;

    if (ctx->role == ROLE_PARAM) return VISIT_SKIP_CHILDREN;

    switch (node->kind) {
    case AST_INT: push_val_ref(c, LLVMConstInt(c->word, node->as.int_val, 0)); break;
    case AST_FUNCTION: cgen_function(c, node); break;
    default: break;
    }

    return VISIT_CONTINUE;
}

static void cgen_leave(AstNode *node, const VisitContext *ctx) {
    Cgen *c = ctx->user_data;
    switch (node->kind) {
    case AST_BINARY: cgen_binary(c, node); break;
    case AST_RETURN: cgen_return(c, node); break;
    default: break;
    }
}

LLVMModuleRef cgen(Cgen *c) {
    AstVisitor v = {.enter = cgen_enter, .leave = cgen_leave};
    ast_walk(c->root, &v, c);
    return c->mod;
}

void free_cgen(Cgen *c) {
    LLVMDisposeBuilder(c->b);
    LLVMDisposeModule(c->mod);
    LLVMContextDispose(c->ctx);
    free((void *)c->stack);
    free(c);
}
