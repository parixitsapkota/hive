#ifndef HIVE_LEXER_H
#define HIVE_LEXER_H

#include <stddef.h>

#include "dep/shi_arena.h"

// Token Kinds
typedef enum {

#define TOKEN(tok) tok,
#define KEYWORDS(tok, name) tok,
#define PUNCTUATION(tok, name) tok,
#include "token.def"
#undef TOKEN
#undef KEYWORDS
#undef PUNCTUATION

    // Misc Tokens
    UNKNOWN_TOKEN = 0,
    END_OF_TOKEN,
} TokenKind;

typedef struct {
    // Position
    size_t ln; // line number;
    size_t cn; // comume number;
} Position;

// Token Defination
typedef struct Token {
    // Value
    TokenKind kind;
    const char *lexeme;
    size_t int_lit;
    // Position
    Position *position;
    // Next Token
    struct Token *next;
} Token;

/// Returns a tokenKind string based on given tokenKind.
char *token_kind_to_str(TokenKind kind);

// Lexer Structure
typedef struct {
    // buffer file name
    const char *file;
    // Input buffer
    const char *buffer;
    size_t buf_len;
    // Position
    size_t i;  // index
    size_t ln; // line number
    size_t cn; // colume number
    // String storage.
    Arena *str_arena;
    size_t srt_data_c;
    // Token List
    Arena *tokens;
    Arena *positions;
    Token *tok_head;
    // Helper/Temp vars
    Token *t_token;
    size_t t_cn;
} Lexer;

#define TOKENS_STORE 1024

/// Returns a lexer context based on given buffer and length of the buffer.
Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len);
/// Lexes based on the given lexer context and mutates the state accordingly.
void lexer(Lexer *l);
/// Frees the allocated memory in the lexing context.
void free_lexer(Lexer *l);

#endif // HIVE_LEXER_H
