#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

bool is_kind_literal(TokenKind kind) {
  switch (kind) {
  case INT:
  case STRING:
  case IDENTIFIER: return true;
  default: return false;
  }
}

bool is_unary_op(TokenKind op) {
  switch (op) {
  case NOT:
  case ADD:
  case SUB:
  case MUL:
  case INC:
  case DEC:
  case BIT_AND: return true;
  default: return false;
  }
}

bool is_proc_left_Associative(Precedence prec) {
  switch (prec) {
  case PREC_ASSIGNMENT: return true;
  default: return false;
  }
}

Precedence get_op_prec(TokenKind kind) {
  switch (kind) {
  case ASSIGN: return PREC_ASSIGNMENT;

  case BIT_OR: return PREC_BIT_OR;

  case BIT_AND: return PREC_BIT_AND;

  case LESSER:
  case GREATER:
  case LESSER_EQUAL:
  case GREATER_EQUAL: return PREC_RELATIONAL;

  case EQUAL:
  case NOT_EQUAL: return PREC_EQUALITY;

  case ADD:
  case SUB: return PREC_ADDITIVE;

  case BITSHIFT_L:
  case BITSHIFT_R: return PREC_BITSHIFT;

  case MUL:
  case DEV:
  case MOD: return PREC_MULTIPLICATIVE;

  case COMMA:
  case C_PREN:
  case C_BRACKET:
  case SEMICOLON: return PREC_NONE;

  default: return PREC_UNKNOWN;
  }
}

AstAtom *parse_atom_f(Parser *p) {
  Token *tok = pconsume(p);
  return new_ast_atom(p->ast, tok->kind, tok->lexeme, tok->int_lit);
}

AstFunctionCall *parse_function_call_s(Parser *p) {
  const char *name = p->tok->lexeme;
  pconsume(p);

  expect_and_consume(p, O_PREN);

  AstNode *body_head = arena_alloc(p->ast, sizeof(AstNode));
  AstNode *body_tail = body_head;

  size_t argc = 0;
  while (p->tok->kind != C_PREN) {
    AstNode *expr = parse_expr_f(p, PREC_NONE);
    add_node(&body_tail, expr);
    ++argc;

    if (is_kind(p, COMMA)) {
      pconsume(p);
    }
  }

  expect_and_consume(p, C_PREN);

  return new_ast_function_call(p->ast, name, body_head->next, argc);
}

AstNode *parse_left_f(Parser *p);

AstUnary *parse_unary_lop_s(Parser *p) {
  TokenKind op = pconsume(p)->kind;
  AstNode *left = parse_left_f(p);
  return new_ast_unary(p->ast, left, op);
}

AstNode *parse_left_f(Parser *p) {
  Token *token = curr(p);

  if (token->kind == O_PREN) {
    expect_and_consume(p, O_PREN);
    AstNode *node = parse_expr_f(p, PREC_NONE);
    expect_and_consume(p, C_PREN);
    return node;
  }

  AstNode *left = arena_alloc(p->ast, sizeof(AstNode));

  if (token->kind == IDENTIFIER && token->next && token->next->kind == O_PREN) {
    *left = (AstNode){AST_FUNCTION_CALL, .function_call_n = parse_function_call_s(p)};
    return left;
  }

  if (is_unary_op(token->kind)) {
    *left = (AstNode){AST_UNARY, .unary_n = parse_unary_lop_s(p)};
    return left;
  }

  *left = (AstNode){AST_ATOM, .atom_n = parse_atom_f(p)};

  if (curr(p)->kind == O_BRACKET) {
    pconsume(p);
    AstNode *index_expr = parse_expr_f(p, PREC_NONE);
    expect_and_consume(p, C_BRACKET);

    AstNode *left_address = arena_alloc(p->ast, sizeof(AstNode));
    *left_address = (AstNode){.kind = AST_UNARY, .unary_n = new_ast_unary(p->ast, left, BIT_AND)};

    AstNode *eight = arena_alloc(p->ast, sizeof(AstNode));
    *eight = (AstNode){.kind = AST_ATOM, .atom_n = new_ast_atom(p->ast, INT, NULL, 8)};

    AstNode *index = arena_alloc(p->ast, sizeof(AstNode));
    *index =
        (AstNode){.kind = AST_BINARY, .binary_n = new_ast_binary(p->ast, index_expr, MUL, eight)};

    AstNode *address = arena_alloc(p->ast, sizeof(AstNode));
    *address = (AstNode){.kind = AST_BINARY,
                         .binary_n = new_ast_binary(p->ast, index, ADD, left_address)};

    AstNode *ret = arena_alloc(p->ast, sizeof(AstNode));
    *ret = (AstNode){.kind = AST_UNARY, .unary_n = new_ast_unary(p->ast, address, MUL)};
    left = ret;
  }

  return left;
}

AstNode *parse_expr_f(Parser *p, Precedence prec) {
  AstNode *left = parse_left_f(p);

  while (curr(p) != NULL) {
    TokenKind op = curr(p)->kind;
    Precedence op_prec = get_op_prec(op);

    if (op_prec == PREC_UNKNOWN) {
      Token *tok = curr(p);
      fprintf(stderr, "%s:%zu:%zu: Unknown operator `%s`.\n", p->l->file, tok->position.ln,
              tok->position.cn, token_kind_to_str(tok->kind));
      exit(EXIT_FAILURE);
    }

    if (op_prec == PREC_NONE || op_prec < prec) {
      break;
    }

    Token *op_tok = pconsume(p);

    AstNode *right;
    if (is_proc_left_Associative(op_prec)) {
      right = parse_expr_f(p, op_prec + 1);
    } else {
      right = parse_expr_f(p, op_prec);
    }

    AstNode *node = arena_alloc(p->ast, sizeof(AstNode));

    *node = (AstNode){AST_BINARY, .binary_n = new_ast_binary(p->ast, left, op, right),
                      .position = position(op_tok->position.ln, op_tok->position.cn)};
    left = node;
  }

  return left;
}
