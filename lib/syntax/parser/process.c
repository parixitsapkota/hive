#include "include/syntax/parser/process.h"
#include "include/syntax/lexer/tokens.h"

Token *current(Parser *p) { return p->tok_tail; }

Token *next(Parser *p) { return p->tok_tail->next; }

Token *pconsume(Parser *p) {
    Token *c = current(p);
    p->tok_tail = p->tok_tail->next;
    return c;
}

bool expect(Parser *p, TokenKind kind) {
    if (pconsume(p)->kind == kind) {
        // TODO: handle the error here
        return false;
    }
    return true;
}

bool expect_and_consume(Parser *p, TokenKind kind) {
    return expect(p, kind) ? pconsume(p) : false;
}

bool is_kind(Parser *p, TokenKind kind) { return current(p)->kind == kind; }
