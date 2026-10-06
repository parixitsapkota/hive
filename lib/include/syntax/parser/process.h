#pragma once

#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"

struct Parser {
    // Token
    Token *tokens;
    Token *tok_tail;
    // AstNode
    AstNode *root;
    AstNode *tail;
    // AstNodes store
    Arena *ast;
};

Token *current(Parser *p);

Token *next(Parser *p);

Token *consume(Parser *p);

bool expect_and_consume(Parser *p, TokenKind kind);

bool is_kind(Parser *p, TokenKind kind);
