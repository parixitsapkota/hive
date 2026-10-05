#include <stdlib.h>

#include "include/core/location.h"
#include "include/syntax/lexer/tokens.h"

const char *token_kind_to_str(TokenKind kind) {
    switch (kind) {
#define TOKEN(tok)                                                                       \
    case TOK_##tok: return #tok;
#define KEYWORDS(tok, name)                                                              \
    case TOK_##tok: return name;
#define PUNCTUATION(tok, name)                                                           \
    case TOK_##tok: return name;
#include "include/syntax/lexer/tokens.def"
#undef TOKEN
#undef KEYWORDS
#undef PUNCTUATION
    default: return "nil";
    }
}

Token *new_token(Arena *arena, TokenKind kind, const char *lexeme, size_t int_lit,
                 Span span) {
    Token *token = arena_alloc(arena, sizeof(Token));
    *token = (Token){kind, lexeme, int_lit, span, NULL};
    return token;
}
