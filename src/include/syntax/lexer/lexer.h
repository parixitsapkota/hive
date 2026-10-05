#pragma once

#include "tokens.h"

#define TOKENS_STORE 1024

typedef struct Lexer Lexer;

Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len);

Token *lexer_lex(Lexer *l);

void free_lexer(Lexer *l);
