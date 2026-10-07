#include <stddef.h>

#include "include/core/location.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/expression.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

static AstNode *parse_statement(Parser *p);

static AstNode *parse_return(Parser *p) {
    Span start = pconsume(p)->span;
    AstNode *expr = parse_expr(p, PREC_NONE);
    Span end = current(p)->span;
    Span span = span_merge(start, end);
    expect_and_consume(p, TOK_SEMICOLON);
    return ast_stmt(p->ast, span, AST_RETURN, expr);
}

static AstNode *parse_body(Parser *p) {
    expect_and_consume(p, TOK_O_BRACE);
    AstNode *first_stmt = NULL;
    AstNode *last_stmt = NULL;

    while (current(p) && current(p)->kind != TOK_C_BRACE) {
        AstNode *statement = parse_statement(p);

        if (!first_stmt) {
            first_stmt = statement;
        } else {
            last_stmt->next = statement;
        }
        last_stmt = statement;
    }
    expect_and_consume(p, TOK_C_BRACE);

    return first_stmt;
}

static AstNode *parse_statement(Parser *p) {
    switch (current(p)->kind) {
    case TOK_RETURN: return parse_return(p);
    case TOK_O_BRACE: return parse_body(p);
    default: {
        AstNode *expr = parse_expr(p, PREC_NONE);
        expect_and_consume(p, TOK_SEMICOLON);
        return expr;
    }
    }
}

AstNode *parse_func(Parser *p) {
    Token *tok = pconsume(p);
    const char *name = tok->lexeme;

    AstNode *first_param = NULL;
    AstNode *last_param = NULL;
    size_t paramc = 0;

    expect_and_consume(p, TOK_O_PREN);
    while (!is_kind(p, TOK_C_PREN)) {
        Token *param = pconsume(p);
        const char *param_name = param->lexeme;
        AstNode *param_ident = ast_ident_val(p->ast, param->span, param_name);
        ++paramc;

        if (!first_param) {
            first_param = param_ident;
        } else {
            last_param->next = param_ident;
        }
        last_param = param_ident;

        if (is_kind(p, TOK_COMMA)) {
            if (next(p) && next(p)->kind == TOK_C_PREN) {
                parser_error(p, "expected a parameter name after `,`");
            }
            pconsume(p);
        } else {
            break;
        }
    }
    Span end = current(p)->span;
    Span span = span_merge(tok->span, end);
    expect_and_consume(p, TOK_C_PREN);

    AstNode *body = parse_statement(p);

    return ast_function(p->ast, span, name, first_param, paramc, body);
}
