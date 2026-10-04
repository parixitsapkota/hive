#ifndef HIVE_AST_H
#define HIVE_AST_H

#include <stddef.h>

#include "dep/shi_arena.h"
#include "dep/shi_hs.h"

#include "lexer.h"

typedef struct AstNode AstNode;

typedef struct {
    TokenKind kind;
    const char *value;
    size_t int_lit;
} AstAtom;

typedef struct {
    AstNode *node;
    TokenKind op;
} AstUnary;

typedef struct {
    AstNode *left;
    TokenKind op;
    AstNode *right;
} AstBinary;

typedef struct AstScope AstScope;
struct AstScope {
    Hs *symtab;
    AstScope *parent;
    AstNode *body;
};

typedef struct {
    const char *name;
    AstNode *body;
    Hs *params_tab;
    Hs *lable_tab;
    size_t params;
} AstFunction;

typedef struct {
    const char *name;
    AstNode *args;
    size_t argc;
} AstFunctionCall;

typedef struct {
    AstNode *Condition;
    AstNode *body;
    AstNode *chain;
} AstIf;

typedef struct {
    AstNode *Condition;
    AstNode *body;
} AstWhile;

typedef struct {
    const char *name;
    size_t size;
} AstVar;

typedef enum {
    AST_ATOM,
    AST_UNARY,
    AST_BINARY,
    AST_EXPR,
    AST_SCOPE,
    AST_FUNCTION,
    AST_FUNCTION_CALL,
    AST_EXTRN,
    AST_AUTO,
    AST_IF,
    AST_ELSE_IF,
    AST_ELSE,
    AST_WHILE,
    AST_RETURN,
    AST_LABLE,
    AST_GOTO,
    AST_GLOBAL,
} AstKind;

struct AstNode {
    AstKind kind;
    // Token Type_union.
    union {
        AstAtom *atom_n;
        AstUnary *unary_n;
        AstBinary *binary_n;
        AstScope *scope_n;
        AstFunction *function_n;
        AstFunctionCall *function_call_n;
        AstIf *if_n;
        AstWhile *while_n;
        AstVar *var_n;
        AstNode *node;
        const char *name_s;
    };
    // Position
    Position *position;
    // for Compound/If statements.
    AstNode *next;
};

typedef enum {
    AUTO_VAR,
    GLOBAL_VAR,
    PARAM_VAR,
    LABLE_S,
} VarKind;

typedef struct {
    VarKind kind;
    size_t offset;
    size_t temp_dest;
} VarInfo;

VarInfo *var_info(Arena *arena, VarKind kind, size_t offset);

AstAtom *new_ast_atom(Arena *arena, TokenKind kind, const char *value, size_t int_lit);
AstUnary *new_ast_unary(Arena *arena, AstNode *node, TokenKind op);
AstBinary *new_ast_binary(Arena *arena, AstNode *left, TokenKind op, AstNode *right);
AstScope *new_ast_scope(Arena *arena, Hs *symtab, AstScope *parent, AstNode *body);
AstFunction *new_ast_function(Arena *arena, const char *name, Hs *params_tab, Hs *lable_tab,
                              size_t params, AstNode *body);
AstFunctionCall *new_ast_function_call(Arena *arena, const char *name, AstNode *args,
                                       size_t argc);
AstIf *new_ast_conditional(Arena *arena, AstNode *Condition, AstNode *body, AstNode *chain);
AstVar *new_ast_var(Arena *arena, const char *name, size_t size);

#endif // HIVE_AST_H
