#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

AstNode *parse_atom(Parser *p) {
    Token *tok = pconsume(p);
    switch (tok->kind) {
    case INT_LIT: return ast_int_val(p->ast, tok->span.start, tok->int_lit);
    case IDENTIFIER_LIT: return ast_ident_val(p->ast, tok->span.start, tok->lexeme);
    case STRING_LIT:
        return ast_string_val(p->ast, tok->span.start, tok->lexeme, tok->int_lit);
    default:
        // TODO: handle the error here
        return NULL;
    }
}
