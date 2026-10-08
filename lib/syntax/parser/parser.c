#include <stdlib.h>

#include "include/core/arena.h"
#include "include/core/diags.h"
#include "include/core/mem.h"
#include "include/core/source.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/statement.h"

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
    if (!p->tokens) return NULL;

    NodeChain chain = {0};

    while (current(p)) {
        Token *tok = current(p);

        if (tok->kind != IDENTIFIER_LIT) {
            parser_error(p, "expected a definition, but found `%s`",
                         token_kind_to_str(tok->kind));
            pconsume(p);
            break;
        }

        Token *nxt = next(p);
        if (nxt && nxt->kind == TOK_O_PREN) {
            chain_append(&chain, parse_func(p));
        } else {
            chain_append(&chain, parse_global_decl(p));
        }
    }
    if (p->errorc > 0) fatal("parser had %zu error(s).", p->errorc);
    return chain.first;
}

void free_parser(Parser *p) {
    free_arena(p->ast);
    free(p);
}
