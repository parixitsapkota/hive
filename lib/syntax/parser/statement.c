#include <stddef.h>

#include "include/core/location.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/expression.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

static AstNode *parse_statement(Parser *p);

static AstNode *parse_if(Parser *p) {
    Span span_start = current(p)->span;
    expect_and_consume(p, TOK_IF);

    expect_and_consume(p, TOK_O_PREN);
    AstNode *condition = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_C_PREN);

    AstNode *then_b = parse_statement(p);

    AstNode *chain = NULL;
    if (is_kind(p, TOK_ELSE)) {
        Span else_start = current(p)->span;
        pconsume(p);
        if (is_kind(p, TOK_IF)) {
            chain = parse_if(p);
        } else {
            AstNode *else_body = parse_statement(p);
            Span end = current(p)->span;
            Span span = span_merge(else_start, end);
            chain = ast_cond(p->ast, span, NULL, else_body, NULL);
        }
    }

    Span end = current(p)->span;
    Span span = span_merge(span_start, end);

    return ast_cond(p->ast, span, condition, then_b, chain);
}

static AstNode *parse_while(Parser *p) {
    Span span_start = current(p)->span;
    pconsume(p);

    expect_and_consume(p, TOK_O_PREN);
    AstNode *condition = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_C_PREN);

    AstNode *body = parse_statement(p);

    Span end = current(p)->span;
    Span span = span_merge(span_start, end);

    return ast_ctl(p->ast, span, AST_WHILE, condition, body);
}

static AstNode *parse_return(Parser *p) {
    Span start = pconsume(p)->span;
    AstNode *expr = parse_expr(p, PREC_NONE);
    Span end = current(p)->span;
    Span span = span_merge(start, end);
    expect_and_consume(p, TOK_SEMICOLON);
    return ast_stmt(p->ast, span, AST_RETURN, expr);
}

static AstNode *parse_break(Parser *p) {
    Span span = pconsume(p)->span;
    expect_and_consume(p, TOK_SEMICOLON);
    return ast_new(p->ast, AST_BREAK, span);
}

static AstNode *parse_continue(Parser *p) {
    Span span = pconsume(p)->span;
    expect_and_consume(p, TOK_SEMICOLON);
    return ast_new(p->ast, AST_CONTINUE, span);
}

static AstNode *parse_label(Parser *p) {
    Token *tok = pconsume(p);
    pconsume(p);
    return ast_label(p->ast, tok->span, AST_LABEL, tok->lexeme);
}

static AstNode *parse_goto(Parser *p) {
    pconsume(p);
    if (current(p)->kind != IDENTIFIER_LIT) {
        parser_error(p, "expected a `IDENTIFIER`, but found `%s`",
                     token_kind_to_str(current(p)->kind));
        return NULL;
    }
    Token *tok = pconsume(p);
    expect_and_consume(p, TOK_SEMICOLON);
    return ast_label(p->ast, tok->span, AST_GOTO, tok->lexeme);
}

static AstNode *parse_case(Parser *p) {
    Span span = pconsume(p)->span;
    AstNode *expr = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_COLON);

    NodeChain statements = {0};
    while (current(p)->kind != TOK_CASE && current(p)->kind != TOK_C_BRACE) {
        chain_append(&statements, parse_statement(p));
    }

    return ast_ctl(p->ast, span, AST_CASE, expr, statements.first);
}

static AstNode *parse_switch_body(Parser *p) {
    expect_and_consume(p, TOK_O_BRACE);

    NodeChain cases = {0};
    while (current(p)->kind != TOK_C_BRACE) {
        chain_append(&cases, parse_case(p));
    }

    expect_and_consume(p, TOK_C_BRACE);
    return cases.first;
}

static AstNode *parse_switch(Parser *p) {
    Span span_start = current(p)->span;
    pconsume(p);

    expect_and_consume(p, TOK_O_PREN);
    AstNode *condition = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_C_PREN);

    AstNode *body = parse_switch_body(p);

    Span end = current(p)->span;
    Span span = span_merge(span_start, end);

    return ast_ctl(p->ast, span, AST_SWITCH, condition, body);
}

typedef struct {
    Token *tok;
    size_t size;
    bool is_vec;
} DeclName;

static AstNode *sync_decl(Parser *p) {
    while (current(p) && current(p)->kind != TOK_SEMICOLON &&
           current(p)->kind != TOK_C_BRACE) {
        pconsume(p);
    }
    if (current(p) && current(p)->kind == TOK_SEMICOLON) pconsume(p);
    return NULL;
}

static bool parse_vector_size(Parser *p, size_t *out) {
    if (!is_kind(p, INT_LIT)) {
        parser_error(p, "expected a constant, before `%s`",
                     current(p) ? token_kind_to_str(current(p)->kind) : "end of input");
        return false;
    }
    *out = (size_t)pconsume(p)->int_lit;
    return expect_and_consume(p, TOK_C_BRACKET);
}

static bool parse_decl_name(Parser *p, bool allow_vector, DeclName *out) {
    if (!is_kind(p, IDENTIFIER_LIT)) {
        parser_error(p, "expected a variable declaration, before `%s`",
                     current(p) ? token_kind_to_str(current(p)->kind) : "end of input");
        return false;
    }
    out->tok = pconsume(p);
    out->size = 1;
    out->is_vec = false;

    if (is_kind(p, TOK_O_BRACKET)) {
        if (!allow_vector) {
            parser_error(p, "`extrn` declarations cannot have a vector size");
            return false;
        }
        pconsume(p);
        out->is_vec = true;
        if (!parse_vector_size(p, &out->size)) return false;
    }
    return true;
}

static AstNode *parse_ival_list(Parser *p) {
    NodeChain ivals = {0};
    for (;;) {
        AstNode *atom = parse_atom(p);
        if (!atom) return NULL;
        chain_append(&ivals, atom);

        if (!is_kind(p, TOK_COMMA)) break;
        pconsume(p);
    }
    return ivals.first;
}

static AstNode *parse_name_list(Parser *p, AstKind kind, bool allow_vector) {
    NodeChain decls = {0};
    for (;;) {
        DeclName name;
        if (!parse_decl_name(p, allow_vector, &name)) return sync_decl(p);

        chain_append(&decls, ast_decl(p->ast, name.tok->span, kind, name.tok->lexeme,
                                      name.is_vec, name.size, NULL));

        if (!is_kind(p, TOK_COMMA)) break;
        pconsume(p);
    }
    if (!expect_and_consume(p, TOK_SEMICOLON)) return sync_decl(p);
    return decls.first;
}

static AstNode *parse_auto_decl(Parser *p) {
    pconsume(p);
    return parse_name_list(p, AST_AUTO, true);
}

static AstNode *parse_extrn_decl(Parser *p) {
    pconsume(p);
    return parse_name_list(p, AST_EXTRN, false);
}

static AstNode *parse_statement(Parser *p);

static AstNode *parse_body(Parser *p) {
    pconsume(p);

    NodeChain stmts = {0};
    while (current(p) && current(p)->kind != TOK_C_BRACE) {
        Token *before = current(p);
        chain_append(&stmts, parse_statement(p));

        if (current(p) == before && current(p)) pconsume(p);
    }
    expect_and_consume(p, TOK_C_BRACE);

    return stmts.first;
}

static AstNode *parse_statement(Parser *p) {
    switch (current(p)->kind) {
    case TOK_BREAK: return parse_break(p);
    case TOK_CONTINUE: return parse_continue(p);
    case TOK_AUTO: return parse_auto_decl(p);
    case TOK_EXTRN: return parse_extrn_decl(p);
    case TOK_RETURN: return parse_return(p);
    case TOK_IF: return parse_if(p);
    case TOK_WHILE: return parse_while(p);
    case TOK_SWITCH: return parse_switch(p);
    case TOK_O_BRACE: return parse_body(p);
    case TOK_GOTO: return parse_goto(p);

    case IDENTIFIER_LIT:
        if (next(p) && next(p)->kind == TOK_COLON) {
            return parse_label(p);
        }
        [[fallthrough]];

    default: {
        AstNode *expr = parse_expr(p, PREC_NONE);
        if (!expr || !expect_and_consume(p, TOK_SEMICOLON)) return sync_decl(p);
        return expr;
    }
    }
}

AstNode *parse_global_decl(Parser *p) {
    DeclName name;
    if (!parse_decl_name(p, true, &name)) return sync_decl(p);

    AstNode *ivals = NULL;

    if (name.is_vec) {
        if (is_kind(p, TOK_COMMA)) {
            pconsume(p);
            ivals = parse_ival_list(p);
            if (!ivals) return sync_decl(p);
        } else if (!is_kind(p, TOK_SEMICOLON)) {
            if (!expect(p, TOK_COMMA)) return sync_decl(p);
        }
    } else {
        bool has_comma = is_kind(p, TOK_COMMA);
        if (has_comma) pconsume(p);
        if (has_comma || !is_kind(p, TOK_SEMICOLON)) {
            ivals = parse_atom(p);
            if (!ivals) return sync_decl(p);
        }
    }

    if (!expect_and_consume(p, TOK_SEMICOLON)) return sync_decl(p);

    return ast_decl(p->ast, name.tok->span, AST_GLOBAL_DECL, name.tok->lexeme,
                    name.is_vec, name.size, ivals);
}

static inline bool check_kind(Parser *p, TokenKind kind) {
    return current(p) && current(p)->kind == kind;
}

static void sync_params(Parser *p) {
    while (current(p) && !check_kind(p, TOK_C_PREN) && !check_kind(p, TOK_O_BRACE) &&
           !check_kind(p, TOK_SEMICOLON)) {
        pconsume(p);
    }
    if (check_kind(p, TOK_C_PREN)) pconsume(p);
}

static bool parse_param_list(Parser *p, NodeChain *params, size_t *paramc) {
    if (check_kind(p, TOK_C_PREN)) return true;

    for (;;) {
        if (!check_kind(p, IDENTIFIER_LIT)) {
            parser_error(p, "expected a parameter name, before `%s`",
                         current(p) ? token_kind_to_str(current(p)->kind)
                                    : "end of input");
            return false;
        }
        Token *param = pconsume(p);
        chain_append(params, ast_ident_val(p->ast, param->span, param->lexeme));
        ++*paramc;

        if (!check_kind(p, TOK_COMMA)) return true;
        pconsume(p);
    }
}

AstNode *parse_func(Parser *p) {
    Token *name_tok = pconsume(p);
    const char *name = name_tok->lexeme;

    NodeChain params = {0};
    size_t paramc = 0;
    Token *close = NULL;

    if (!expect_and_consume(p, TOK_O_PREN) || !parse_param_list(p, &params, &paramc)) {
        sync_params(p);
    } else if (check_kind(p, TOK_C_PREN)) {
        close = pconsume(p);
    } else {
        expect(p, TOK_C_PREN);
        sync_params(p);
    }

    Span span = close ? span_merge(name_tok->span, close->span) : name_tok->span;
    AstNode *body = parse_statement(p);

    return ast_function(p->ast, span, name, params.first, paramc, body);
}
