#include <stdlib.h>

#include "include/core/arena.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/expression.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

Parser *init_parser(Token *tokens) {
    Parser *p = malloc(sizeof(Parser));
    *p = (Parser){0};
    p->tokens = tokens;
    p->tok_tail = tokens;
    p->ast = init_arena(1024 * (sizeof(AstNode)));
    return p;
}

AstNode *parser_parse(Parser *p) {
    if (!p->tokens) {
        return NULL;
    }

    print_tokens(p->tokens->span.start->file_path, p->tokens);

    AstNode *node = parse_atom(p);

    return node;
}

void free_parser(Parser *p) {
    free_arena(p->ast);
    free(p);
}
