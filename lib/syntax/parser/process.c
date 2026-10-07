#include <stdarg.h>
#include <stdbool.h>

#include "include/core/diags.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/process.h"

Token *current(Parser *p) { return p->tok_tail; }

Token *next(Parser *p) { return p->tok_tail->next; }

Token *pconsume(Parser *p) {
    Token *c = current(p);
    p->tok_tail = p->tok_tail->next;
    return c;
}

bool expect(Parser *p, TokenKind kind) {
    Token *tok = current(p);

    if (!tok || tok->kind != kind) {
        const char *got =
            tok ? (tok->lexeme ? tok->lexeme : token_kind_to_str(tok->kind)) : "EOF";
        diag_err(p->src, tok ? tok->span : (Span){0}, "expected `%s`, but found `%s`",
                 token_kind_to_str(kind), got);
        return false;
    }

    return true;
}

bool expect_and_consume(Parser *p, TokenKind kind) {
    if (!expect(p, kind)) {
        return false;
    }
    pconsume(p);
    return true;
}

bool is_kind(Parser *p, TokenKind kind) { return current(p)->kind == kind; }

void *parser_error(Parser *p, const char *fmt, ...) {
    ++p->errorc;
    va_list args;
    va_start(args, fmt);
    vdiag_err(p->src, p->tok_tail->span, fmt, args);
    va_end(args);
    return NULL;
}
