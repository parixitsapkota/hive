#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "include/core/arena.h"
#include "lexer.h"
#include "tokens.h"

struct Lexer {
    bool had_error;
    // buffer file name
    const char *file_path;
    // Input buffer
    const char *buffer;
    size_t buf_len;
    // Position
    size_t i;  // index
    size_t ln; // line number
    size_t cn; // colume number
    // String storage.
    Arena *lexeme;
    size_t srt_data_c;
    // Token List
    Arena *tokens;
    Arena *locations;
    Token *tok_head;
    // Helper/Temp vars
    Token *t_token;
    size_t t_cn;
};

typedef struct Mark {
    size_t offset;
    size_t ln;
    size_t cn;
} Mark;

typedef struct {
    const char *text;
    size_t len;
    TokenKind kind;
} Punctuator;

bool is_space(char c);
bool is_alpha(char c);
bool is_digit(char c);

bool is_at_end(const Lexer *l);
char peek(const Lexer *l, size_t offset);
void consume(Lexer *l, size_t count);
Mark mark(const Lexer *l);

void error(Lexer *l, Mark at, const char *fmt, ...);

void skip_block_comment(Lexer *l, Mark start);
void lex_identifier(Lexer *l, Mark start);
void lex_number(Lexer *l, Mark start);
void lex_quoted(Lexer *l, Mark start);
bool lex_punctuator(Lexer *l, Mark start);
