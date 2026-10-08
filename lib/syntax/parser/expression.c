#include <stdbool.h>
#include <stddef.h>

#include "include/core/location.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/expression.h"
#include "include/syntax/parser/parser.h"
#include "include/syntax/parser/process.h"

static AstNode *parse_primary(Parser *p);

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
    case TOK_INC:
    case TOK_DEC: return true;
    default: return false;
    }
}

static bool is_unary_prefix(TokenKind kind) {
    switch (kind) {
    case TOK_NOT:
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_BIT_AND: return true;
    default: return is_unary_op(kind);
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
    case TOK_COLON:
    case TOK_SEMICOLON: return true;

    default: return false;
    }
}

AstNode *parse_atom(Parser *p) {
    Token *tok = current(p);
    if (is_kind_literal(tok->kind)) tok = pconsume(p);

    switch (tok->kind) {
    case INT_LIT: return ast_int_val(p->ast, tok->span, tok->int_lit);
    case IDENTIFIER_LIT: return ast_ident_val(p->ast, tok->span, tok->lexeme);
    case STRING_LIT: return ast_string_val(p->ast, tok->span, tok->lexeme, tok->int_lit);
    default:
        return parser_error(p, "expected a literal, but got `%s`",
                            token_kind_to_str(tok->kind));
    }
}

AstNode *parse_paren_expr(Parser *p) {
    expect_and_consume(p, TOK_O_PREN);

    if (is_expression_delimiter(current(p)->kind)) {
        parser_error(p, "expected a expression");
        pconsume(p);
        return NULL;
    }

    AstNode *node = parse_expr(p, PREC_NONE);
    expect_and_consume(p, TOK_C_PREN);
    return node;
}

/* ---------- subscript: target '[' expr ']' ---------- */
static AstNode *parse_subscript(Parser *p, AstNode *target) {
    pconsume(p); /* '[' */
    AstNode *index = parse_expr(p, PREC_NONE);

    Span span = span_merge(target->span, current(p)->span);
    expect_and_consume(p, TOK_C_BRACKET);

    return ast_index(p->ast, span, target, index);
}

/* ---------- postfix: primary { subscript | ++ | -- } ----------
 * Left-associative, binds tighter than any prefix operator.
 * Calls are handled in parse_primary (name '(' ... ')'), so they
 * are already a primary here:
 *   f(x)++--   ->  ((f(x))++)--
 *   a[i]++--   ->  ((a[i])++)--
 */
static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);
    if (!expr) return NULL;

    for (;;) {
        TokenKind kind = current(p)->kind;

        if (kind == TOK_O_BRACKET) {
            expr = parse_subscript(p, expr);
        } else if (kind == TOK_INC || kind == TOK_DEC) {
            Token *op = pconsume(p);
            Span span = span_merge(expr->span, op->span);
            expr = ast_unary(p->ast, span, op->kind, true, expr);
        } else {
            break;
        }
    }
    return expr;
}

/* ---------- unary (prefix) ----------
 * Right-associative: ++--x, -*p, !-x, &a[i], ...
 * The operand is another unary expression (which bottoms out in postfix),
 * NOT parse_expr, so `-a + b` is `(-a) + b` and `++a[i]++` is `++(a[i]++)`.
 */
static AstNode *parse_unary(Parser *p) {
    if (is_unary_prefix(current(p)->kind)) {
        Token *tok = pconsume(p);
        Span op_span = tok->span; /* copy before parsing further */
        TokenKind op = tok->kind;

        AstNode *operand = parse_unary(p);
        if (!operand) return NULL;

        Span span = span_merge(op_span, operand->span);
        return ast_unary(p->ast, span, op, false, operand);
    }
    return parse_postfix(p);
}

AstNode *parse_call(Parser *p) {
    Token *tok = pconsume(p);
    const char *name = tok->lexeme;
    expect_and_consume(p, TOK_O_PREN);

    size_t argc = 0;
    AstNode *first_arg = NULL;
    AstNode *last_arg = NULL;

    while (!is_kind(p, TOK_C_PREN)) {
        AstNode *expr = parse_expr(p, PREC_NONE);
        ++argc;

        if (!first_arg) {
            first_arg = expr;
        } else {
            last_arg->next = expr;
        }
        last_arg = expr;

        if (is_kind(p, TOK_COMMA)) {
            if (next(p)) {
                if (next(p)->kind == TOK_C_PREN)
                    parser_error(p, "expected a expression after `,`");
            }
            pconsume(p);
        } else {
            break;
        }
    }
    Span span = span_merge(tok->span, current(p)->span);
    expect_and_consume(p, TOK_C_PREN);

    return ast_call(p->ast, span, name, first_arg, argc);
}

/* ---------- primary: call | '(' expr ')' | atom ----------
 * No prefix handling here; prefix sits ABOVE postfix.
 * A call is `identifier '('`, detected by one token of lookahead.
 */
static AstNode *parse_primary(Parser *p) {
    TokenKind kind = current(p)->kind;

    if (kind == IDENTIFIER_LIT && next(p) && next(p)->kind == TOK_O_PREN) {
        return parse_call(p);
    }
    if (kind == TOK_O_PREN) {
        return parse_paren_expr(p);
    }
    return parse_atom(p);
}

AstNode *parse_expr(Parser *p, Precedence prec) {
    if (current(p)->kind == TOK_SEMICOLON) return NULL;

    Span span_start = current(p)->span;

    AstNode *left = parse_unary(p);
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
        bool assignment = is_proc_left_Associative(op_prec);
        Precedence next_prec = assignment ? op_prec + 1 : op_prec;
        AstNode *right = parse_expr(p, next_prec);

        if (!right) return NULL;

        Span span_end = current(p)->span;
        Span span = span_merge(span_start, span_end);

        AstKind kind = assignment ? AST_ASSIGN : AST_BINARY;
        left = ast_binary(p->ast, span, kind, op, left, right);
    }
    }

    return left;
}
