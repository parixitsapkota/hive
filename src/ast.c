#include "dep/shi_arena.h"
#include <stddef.h>

#include "ast.h"

VarInfo *var_info(Arena *arena, VarKind kind, size_t offset) {
  VarInfo *var = arena_alloc(arena, sizeof(VarInfo));
  *var = (VarInfo){.kind = kind, .offset = offset};
  return var;
}

AstAtom *new_ast_atom(Arena *arena, TokenKind kind, const char *value, size_t int_lit) {
  AstAtom *atom = arena_alloc(arena, sizeof(AstAtom));
  *atom = (AstAtom){.kind = kind, .value = value, .int_lit = int_lit};
  return atom;
}

AstUnary *new_ast_unary(Arena *arena, AstNode *node, TokenKind op) {
  AstUnary *unary = arena_alloc(arena, sizeof(AstUnary));
  *unary = (AstUnary){.node = node, .op = op};
  return unary;
}

AstBinary *new_ast_binary(Arena *arena, AstNode *left, TokenKind op, AstNode *right) {
  AstBinary *binary = arena_alloc(arena, sizeof(AstBinary));
  *binary = (AstBinary){.left = left, .op = op, .right = right};
  return binary;
}

AstScope *new_ast_scope(Arena *arena, Hs *symtab, AstScope *parent, AstNode *body) {
  AstScope *scope = arena_alloc(arena, sizeof(AstScope));
  *scope = (AstScope){.symtab = symtab, .parent = parent, .body = body};
  return scope;
}

AstFunction *new_ast_function(Arena *arena, const char *name, Hs *params_tab, Hs *lable_tab,
                              size_t params, AstNode *body) {
  AstFunction *function = arena_alloc(arena, sizeof(AstFunction));
  *function = (AstFunction){.name = name,
                            .params_tab = params_tab,
                            .lable_tab = lable_tab,
                            .params = params,
                            .body = body};
  return function;
}

AstFunctionCall *new_ast_function_call(Arena *arena, const char *name, AstNode *args,
                                       size_t argc) {
  AstFunctionCall *function_call = arena_alloc(arena, sizeof(AstFunctionCall));
  *function_call = (AstFunctionCall){.name = name, .args = args, .argc = argc};
  return function_call;
}

AstIf *new_ast_conditional(Arena *arena, AstNode *Condition, AstNode *body, AstNode *chain) {
  AstIf *if_n = arena_alloc(arena, sizeof(AstIf));
  *if_n = (AstIf){.Condition = Condition, .body = body, .chain = chain};
  return if_n;
}

AstVar *new_ast_var(Arena *arena, const char *name, size_t size) {
  AstVar *global_n = arena_alloc(arena, sizeof(AstVar));
  *global_n = (AstVar){.name = name, .size = size};
  return global_n;
}
