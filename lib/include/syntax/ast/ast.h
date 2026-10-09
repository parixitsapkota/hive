#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <llvm-c/Core.h>

#include "include/core/arena.h"
#include "include/core/location.h"
#include "include/syntax/lexer/tokens.h"

typedef struct AstNode AstNode;

typedef enum {
    VAR_AUTO,
    VAR_GLOBAL,
    VAR_PARAM,
    VAR_EXTRN,
    VAR_LABEL,
} VarKind;

typedef struct {
    VarKind kind;
    LLVMValueRef llvm_vl_ref;
} VarInfo;

typedef enum {
    /* expressions */
    AST_INT,
    AST_CHAR,
    AST_STRING,
    AST_IDENT,
    AST_UNARY,
    AST_BINARY,
    AST_ASSIGN,
    AST_TERNARY,
    AST_INDEX,
    AST_CALL,

    /* statements */
    AST_BLOCK,
    AST_EXPR_STMT,
    AST_NULL_STMT,
    AST_IF,
    AST_WHILE,
    AST_SWITCH,
    AST_CASE,
    AST_BREAK,
    AST_CONTINUE,
    AST_RETURN,
    AST_GOTO,
    AST_LABEL,

    /* declarations */
    AST_AUTO,
    AST_EXTRN,
    AST_GLOBAL_DECL,
    AST_FUNCTION,
} AstKind;

struct AstNode {
    AstKind kind;
    Span span;
    AstNode *next;

    union {
        /* AST_INT, AST_CHAR */
        size_t int_val;

        /* AST_STRING */
        struct {
            const char *str;
            size_t string_num;
        } string;

        /* AST_IDENT */
        struct {
            const char *name;
            VarInfo *info;
        } ident;

        /* AST_UNARY */
        struct {
            TokenKind op;
            bool postfix;
            AstNode *operand;
        } unary;

        /* AST_BINARY, AST_ASSIGN */
        struct {
            TokenKind op;
            AstNode *lhs;
            AstNode *rhs;
        } binary;

        /* AST_IF, AST_TERNARY */
        struct {
            AstNode *cond;
            AstNode *then_b;
            AstNode *else_b;
        } cond;

        /* AST_WHILE, AST_SWITCH, AST_CASE (expr is a constant for case) */
        struct {
            AstNode *expr;
            AstNode *body;
        } ctl;

        /* AST_INDEX */
        struct {
            AstNode *base;
            AstNode *index;
        } index;

        /* AST_CALL: args is a linked list */
        struct {
            const char *callee;
            AstNode *args;
            size_t argc;
        } call;

        /* AST_BLOCK: stmts is a linked list */
        struct {
            AstNode *stmts;
        } block;

        /* AST_EXPR_STMT, AST_RETURN (expr may be NULL) */
        struct {
            AstNode *expr;
        } stmt;

        /* AST_LABEL, AST_GOTO */
        const char *label;

        /* AST_AUTO, AST_EXTRN, AST_GLOBAL_DECL
         * multiple decleration are a linked list */
        struct {
            const char *name;
            bool is_vec;
            size_t size;
            AstNode *init;
            VarInfo *info;
        } decl;

        /* AST_FUNCTION */
        struct {
            const char *name;
            AstNode *params;
            size_t nparams;
            AstNode *body;
        } func;
    } as;
};

VarInfo *var_info(Arena *arena, VarKind kind);

AstNode *ast_new(Arena *arena, AstKind kind, Span span);

AstNode *ast_int_val(Arena *a, Span span, size_t int_val);
AstNode *ast_string_val(Arena *a, Span span, const char *str, size_t str_num);
AstNode *ast_ident_val(Arena *a, Span span, const char *name);

AstNode *ast_unary(Arena *a, Span span, TokenKind op, bool postfix, AstNode *operand);
AstNode *ast_binary(Arena *a, Span span, AstKind kind, TokenKind op, AstNode *l,
                    AstNode *r);

AstNode *ast_stmt(Arena *a, Span span, AstKind kind, AstNode *expr);
AstNode *ast_label(Arena *a, Span span, AstKind kind, const char *name);
AstNode *ast_index(Arena *a, Span span, AstNode *left, AstNode *index);
AstNode *ast_call(Arena *a, Span span, const char *callee, AstNode *args, size_t argc);
AstNode *ast_cond(Arena *a, Span span, AstNode *cond, AstNode *then_b, AstNode *else_b);
AstNode *ast_ctl(Arena *a, Span span, AstKind kind, AstNode *cond, AstNode *body);
AstNode *ast_block(Arena *a, Span span, AstNode *stmts);
AstNode *ast_decl(Arena *a, Span span, AstKind kind, const char *name, bool is_vec,
                  size_t size, AstNode *init);

AstNode *ast_function(Arena *a, Span span, const char *name, AstNode *params,
                      size_t nparams, AstNode *body);

void print_ast(AstNode *root);
