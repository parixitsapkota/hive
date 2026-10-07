#pragma once

#include "include/core/source.h"
#include "tokens.h"

#define TOKENS_STORE 1024

typedef struct Lexer Lexer;

Lexer *init_lexer(const Source *src);

Token *lexer_lex(Lexer *l);

void free_lexer(Lexer *l);
