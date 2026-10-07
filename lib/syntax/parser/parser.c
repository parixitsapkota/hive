#include <stdio.h>
#include <stdlib.h>

#include "include/core/arena.h"
#include "include/core/diags.h"
#include "include/core/mem.h"
#include "include/core/source.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/expression.h"
#include "include/syntax/parser/parser.h"

#include "include/syntax/parser/process.h"

Parser *init_parser(const Source *src, Token *tokens) {
    Parser *p = xmalloc(sizeof(Parser));
    *p = (Parser){0};
    p->tokens = tokens;
    p->src = src;
    p->tok_tail = tokens;
    p->ast = init_arena(1024 * (sizeof(AstNode)));
    return p;
}

AstNode *parser_parse(Parser *p) {
    if (!p->tokens) {
        return NULL;
    }

    // print_tokens(p->src->file_path, p->tokens);

    AstNode *node = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_SEMICOLON);

    print_ast(node);

    if (p->errorc > 0) fatal("parser had %zu error(s).", p->errorc);
    return node;
}

void free_parser(Parser *p) {
    free_arena(p->ast);
    free(p);
}
