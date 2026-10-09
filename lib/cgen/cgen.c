#include <assert.h>
#include <stdbool.h>
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
#include "include/syntax/lexer/tokens.h"

#define CGEN_STACK_INIT 64

struct Cgen {
    Source *src;
    AstNode *root;
    bool is_lvalue;
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
    LLVMBasicBlockRef entry_block;
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

static LLVMValueRef build_arith(Cgen *c, TokenKind op, LLVMValueRef l, LLVMValueRef r) {
    switch (op) {
    case TOK_ADD: return LLVMBuildAdd(c->b, l, r, "");
    case TOK_SUB: return LLVMBuildSub(c->b, l, r, "");
    case TOK_MUL: return LLVMBuildMul(c->b, l, r, "");
    case TOK_DEV: return LLVMBuildSDiv(c->b, l, r, "");
    case TOK_MOD: return LLVMBuildSRem(c->b, l, r, "");

    case TOK_BIT_AND: return LLVMBuildAnd(c->b, l, r, "");
    case TOK_BIT_OR: return LLVMBuildOr(c->b, l, r, "");

    case TOK_GREATER: {
        LLVMValueRef cmp = LLVMBuildICmp(c->b, LLVMIntSGT, l, r, "");
        return LLVMBuildZExt(c->b, cmp, c->word, "");
    }
    case TOK_LESSER: {
        LLVMValueRef cmp = LLVMBuildICmp(c->b, LLVMIntSLT, l, r, "");
        return LLVMBuildZExt(c->b, cmp, c->word, "");
    }
    default: return NULL;
    }
}

static void cgen_binary(Cgen *c, AstNode *node) {
    LLVMValueRef rhs = pop_val_ref(c);
    LLVMValueRef lhs = pop_val_ref(c);
    LLVMValueRef r = build_arith(c, node->as.binary.op, lhs, rhs);
    assert(r && "cgen: unsupported binary operator");
    push_val_ref(c, r);
}

static void cgen_assign(Cgen *c, AstNode *node) {
    LLVMValueRef rhs = pop_val_ref(c);
    LLVMValueRef addr = pop_val_ref(c);
    LLVMValueRef result = rhs;

    if (node->as.binary.op != TOK_ASSIGN) {
        LLVMValueRef old = LLVMBuildLoad2(c->b, c->word, addr, "");
        result = build_arith(c, node->as.binary.op, old, rhs);
        assert(result && "cgen: unsupported compound assignment operator");
    }

    LLVMBuildStore(c->b, result, addr);
    push_val_ref(c, result);
}

static void cgen_auto(Cgen *c, AstNode *node) {
    LLVMValueRef i = LLVMBuildAlloca(c->b, c->word, node->as.decl.name);
    node->as.decl.info->llvm_vl_ref = i;
}

static void cgen_return(Cgen *c, AstNode *node) {
    LLVMValueRef v = node->as.stmt.expr ? pop_val_ref(c) : LLVMConstInt(c->word, 0, 0);
    LLVMBuildRet(c->b, v);
}

static LLVMValueRef cgen_rval(Cgen *c, AstNode *node) {
    switch (node->kind) {
    case AST_IDENT: {
        LLVMValueRef slot = node->as.ident.info->llvm_vl_ref;
        return LLVMBuildLoad2(c->b, c->word, slot, "");
    }
    case AST_INT: return LLVMConstInt(c->word, node->as.int_val, 0);
    default: return 0;
    }
    (void)c;
}

static LLVMValueRef cgen_lval(Cgen *c, AstNode *node) {
    switch (node->kind) {
    case AST_IDENT: return node->as.ident.info->llvm_vl_ref;
    case AST_UNARY: {
        if (node->as.unary.op == TOK_MUL) {
            return cgen_rval(c, node->as.unary.operand);
        }
        break;
    }
    default: break;
    }
    return 0;
    (void)c;
}

static void cgen_leave(AstNode *node, const VisitContext *ctx) {
    Cgen *c = ctx->user_data;
    switch (node->kind) {
    case AST_BINARY: cgen_binary(c, node); break;
    case AST_ASSIGN: cgen_assign(c, node); break;
    case AST_RETURN: cgen_return(c, node); break;
    default: break;
    }
}

static VisitAction cgen_enter(AstNode *node, const VisitContext *ctx) {
    Cgen *c = ctx->user_data;

    if (ctx->role == ROLE_PARAM) return VISIT_SKIP_CHILDREN;

    switch (node->kind) {
    case AST_ASSIGN: {
        push_val_ref(c, cgen_lval(c, node->as.binary.lhs));

        ast_walk(node->as.binary.rhs,
                 &(AstVisitor){.enter = cgen_enter, .leave = cgen_leave}, c);
        return VISIT_SKIP_CHILDREN;
    }

    case AST_IDENT:
    case AST_INT: push_val_ref(c, cgen_rval(c, node)); break;
    case AST_AUTO: cgen_auto(c, node); break;
    case AST_FUNCTION: cgen_function(c, node); break;
    default: break;
    }

    return VISIT_CONTINUE;
}

void cgen(Cgen *c) {
    AstVisitor v = {.enter = cgen_enter, .leave = cgen_leave};
    ast_walk(c->root, &v, c);

    char *ir = LLVMPrintModuleToString(c->mod);
    puts("\n");
    puts(ir);
    LLVMDisposeMessage(ir);
}

void free_cgen(Cgen *c) {
    LLVMDisposeBuilder(c->b);
    LLVMDisposeModule(c->mod);
    LLVMContextDispose(c->ctx);
    free((void *)c->stack);
    free(c);
}

bool cgen_emit_object(Cgen *c, const char *path) {
    char *err = NULL;

    LLVMBool failed = LLVMVerifyModule(c->mod, LLVMReturnStatusAction, &err);
    if (failed) {
        fprintf(stderr, "verify failed: %s\n", err);
    }

    if (err) {
        LLVMDisposeMessage(err);
        err = NULL;
    }
    if (failed) {
        return true;
    }

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    char *triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
        fprintf(stderr, "target error: %s\n", err);
        if (err) LLVMDisposeMessage(err);
        LLVMDisposeMessage(triple);
        return true;
    }

    LLVMTargetMachineRef tm =
        LLVMCreateTargetMachine(target, triple, "generic", "", LLVMCodeGenLevelDefault,
                                LLVMRelocPIC, LLVMCodeModelDefault);

    LLVMTargetDataRef layout = LLVMCreateTargetDataLayout(tm);
    LLVMSetTarget(c->mod, triple);
    LLVMSetModuleDataLayout(c->mod, layout);

    bool rc = false;
    if (LLVMTargetMachineEmitToFile(tm, c->mod, (char *)path, LLVMObjectFile, &err)) {
        fprintf(stderr, "emit error: %s\n", err);
        if (err) LLVMDisposeMessage(err);
        rc = true;
    }

    LLVMDisposeTargetData(layout);
    LLVMDisposeTargetMachine(tm);
    LLVMDisposeMessage(triple);
    return rc;
}
