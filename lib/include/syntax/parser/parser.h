#pragma once

#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"

typedef struct Parser Parser;

Parser *init_parser(const char *file_name, Token *tokens);

AstNode *parser_parse(Parser *p);

void free_parser(Parser *p);
