#include "include/syntax/parser/expression.h"
#include "include/core/location.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"
#include <stdbool.h>

static bool is_kind_literal(TokenKind kind) {
    switch (kind) {
    case INT_LIT:
    case STRING_LIT:
    case IDENTIFIER_LIT: return true;
    default: return false;
    }
}

static bool is_unary_op(TokenKind kind) {
    switch (kind) {
    case TOK_NOT:
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_INC:
    case TOK_DEC:
    case TOK_BIT_AND: return true;
    default: return false;
    }
}

static bool is_proc_left_Associative(Precedence prec) {
    switch (prec) {
    case PREC_ASSIGNMENT: return true;
    default: return false;
    }
}

static Precedence get_op_prec(TokenKind kind) {
    switch (kind) {
    case TOK_ASSIGN: return PREC_ASSIGNMENT;

    case TOK_BIT_OR: return PREC_BIT_OR;

    case TOK_BIT_AND: return PREC_BIT_AND;

    case TOK_LESSER:
    case TOK_GREATER:
    case TOK_LESSER_EQUAL:
    case TOK_GREATER_EQUAL: return PREC_RELATIONAL;

    case TOK_EQUAL:
    case TOK_NOT_EQUAL: return PREC_EQUALITY;

    case TOK_ADD:
    case TOK_SUB: return PREC_ADDITIVE;

    case TOK_BITSHIFT_L:
    case TOK_BITSHIFT_R: return PREC_BITSHIFT;

    case TOK_MUL:
    case TOK_DEV:
    case TOK_MOD: return PREC_MULTIPLICATIVE;

    case TOK_COMMA:
    case TOK_C_PREN:
    case TOK_C_BRACKET:
    case TOK_SEMICOLON: return PREC_NONE;

    default: return PREC_UNKNOWN;
    }
}

static bool is_expression_delimiter(TokenKind kind) {
    switch (kind) {
    case TOK_COMMA:
    case TOK_C_PREN:
    case TOK_C_BRACKET:
    case TOK_SEMICOLON: return true;

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
    default:
        return parser_error(p, "expected a literal, but got `%s`",
                            token_kind_to_str(tok->kind));
    }
}

AstNode *parse_paren_expr(Parser *p) {
    expect_and_consume(p, TOK_O_PREN);
    AstNode *node = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_C_PREN);
    return node;
}

AstNode *parse_unary_prefix(Parser *p) {
    TokenKind op = pconsume(p)->kind;
    AstNode *node = parse_expr(p, PREC_NONE);
    return ast_unary(p->ast, NULL, op, false, node);
}

AstNode *parse_primary(Parser *p) {
    TokenKind kind = current(p)->kind;
    if (kind == TOK_O_PREN) return parse_paren_expr(p);
    if (is_unary_op(kind)) return parse_unary_prefix(p);
    return parse_atom(p);
}

AstNode *parse_expr(Parser *p, Precedence prec) {
    AstNode *left = parse_primary(p);
    if (!left) return NULL;

    while (current(p) != NULL) {
        Token *tok = current(p);
        TokenKind op = tok->kind;
        Precedence op_prec = get_op_prec(op);

        if (is_expression_delimiter(tok->kind)) break;
        if (op_prec == PREC_UNKNOWN) {
            const char *got = token_kind_to_str(tok->kind);
            parser_error(p, "expected `;` before `%s`", got);
            op_prec = PREC_ADDITIVE;
            op = TOK_ADD;
            goto IF_FAKE_OP;
        }
        if (op_prec == PREC_NONE || op_prec < prec) break;

        pconsume(p);
    IF_FAKE_OP: {
        Precedence next_prec = is_proc_left_Associative(op_prec) ? op_prec + 1 : op_prec;
        AstNode *right = parse_expr(p, next_prec);

        if (!right) return NULL;

        left = ast_binary(p->ast, NULL, AST_BINARY, op, left, right);
    }
    }

    return left;
}
