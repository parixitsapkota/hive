#include <stdlib.h>

#include "include/syntax/lexer/tokens.h"

const char *token_kind_to_str(TokenKind kind) {
    switch (kind) {
#define TOKEN(tok)                                                                       \
    case tok: return #tok;
#define KEYWORDS(tok, name)                                                              \
    case tok: return name;
#define PUNCTUATION(tok, name)                                                           \
    case tok: return name;
#include "include/syntax/lexer/tokens.def"
#undef TOKEN
#undef KEYWORDS
#undef PUNCTUATION
    default: return "nil";
    }
}

Token *new_token(Arena *arena, TokenKind kind, const char *lexeme, size_t int_lit,
                 Location *location) {
    Token *token = arena_alloc(arena, sizeof(Token));
    *token = (Token){kind, lexeme, int_lit, location, NULL};
    return token;
}
