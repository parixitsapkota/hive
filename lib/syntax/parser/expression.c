#include "include/syntax/parser/expression.h"
#include "include/core/diags.h"
#include "include/core/location.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

static bool is_kind_literal(TokenKind kind) {
    switch (kind) {
    case INT_LIT:
    case STRING_LIT:
    case IDENTIFIER_LIT: return true;
    default: return false;
    }
}

AstNode *parse_atom(Parser *p) {
    Token *tok = current(p);
    if (is_kind_literal(tok->kind)) tok = pconsume(p);

    switch (tok->kind) {
    case INT_LIT: return ast_int_val(p->ast, tok->span.start, tok->int_lit);
    case IDENTIFIER_LIT: return ast_ident_val(p->ast, tok->span.start, tok->lexeme);
    case STRING_LIT:
        return ast_string_val(p->ast, tok->span.start, tok->lexeme, tok->int_lit);
    default: {
        const char *got = token_kind_to_str(tok->kind);
        diag_err(p->src, tok->span, "expected a literal, but got `%s`", got);
        return NULL;
    }
    }
}
