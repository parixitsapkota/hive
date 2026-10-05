#pragma once

#include "include/core/arena.h"
#include "include/core/location.h"

typedef enum {

#define TOKEN(tok) TOK_##tok,
#define KEYWORDS(tok, name) TOK_##tok,
#define PUNCTUATION(tok, name) TOK_##tok,
#include "tokens.def"
#undef TOKEN
#undef KEYWORDS
#undef PUNCTUATION

    // Misc Tokens
    UNKNOWN_TOKEN = 0,
    END_OF_TOKEN,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    const char *lexeme;
    size_t int_lit;
    Span span;
    struct Token *next;
} Token;

const char *token_kind_to_str(TokenKind kind);
Token *new_token(Arena *arena, TokenKind kind, const char *lexeme, size_t int_lit,
                 Span span);
