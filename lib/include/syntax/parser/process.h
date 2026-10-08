#pragma once

#include "include/core/source.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"
#include <stddef.h>

struct Parser {
    const Source *src;
    // Token
    Token *tokens;
    Token *tok_tail;
    // AstNode
    AstNode *root;
    AstNode *tail;
    // AstNodes store
    Arena *ast;
    // error
    size_t errorc;
};


typedef struct {
    AstNode *first;
    AstNode *last;
} NodeChain;

void chain_append(NodeChain *c, AstNode *head);

Token *current(Parser *p);

Token *next(Parser *p);

Token *pconsume(Parser *p);

bool expect(Parser *p, TokenKind kind);

bool expect_and_consume(Parser *p, TokenKind kind);

bool is_kind(Parser *p, TokenKind kind);

void *parser_error(Parser *p, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
