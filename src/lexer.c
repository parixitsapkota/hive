#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dep/keywords.h"
#include "lexer.h"

typedef struct {
    size_t offset;
    size_t ln;
    size_t cn;
} Mark;

typedef struct {
    const char *text;
    size_t len;
    TokenKind kind;
} Punctuator;

#define PUNCT(s, k) {s, sizeof(s) - 1, k}

static const Punctuator punctuators[] = {
    PUNCT("<<", BITSHIFT_L), PUNCT("<=", LESSER_EQUAL),
    PUNCT(">>", BITSHIFT_R), PUNCT(">=", GREATER_EQUAL),
    PUNCT("++", INC),        PUNCT("--", DEC),
    PUNCT("!=", NOT_EQUAL),  PUNCT("==", EQUAL),

    PUNCT("{", O_BRACE),     PUNCT("}", C_BRACE),
    PUNCT("[", O_BRACKET),   PUNCT("]", C_BRACKET),
    PUNCT("(", O_PREN),      PUNCT(")", C_PREN),
    PUNCT(";", SEMICOLON),   PUNCT(":", COLON),
    PUNCT("?", Q_MARK),      PUNCT("&", BIT_AND),
    PUNCT("|", BIT_OR),      PUNCT(",", COMMA),
    PUNCT("*", MUL),         PUNCT("/", DEV),
    PUNCT("%", MOD),         PUNCT("+", ADD),
    PUNCT("-", SUB),         PUNCT("!", NOT),
    PUNCT("=", ASSIGN),      PUNCT("<", LESSER),
    PUNCT(">", GREATER),
};

#undef PUNCT

static bool had_error = false;

static bool is_space(char c) { return isspace((unsigned char)c) != 0; }
static bool is_alpha(char c) { return isalpha((unsigned char)c) != 0; }
static bool is_digit(char c) { return isdigit((unsigned char)c) != 0; }
static bool is_xdigit(char c) { return isxdigit((unsigned char)c) != 0; }
static bool is_bin_digit(char c) { return c == '0' || c == '1'; }
static bool is_octal(char c) { return c >= '0' && c <= '7'; }
static bool is_ident_char(char c) { return isalnum((unsigned char)c) != 0 || c == '_'; }

static bool is_at_end(const Lexer *l) { return l->i >= l->buf_len; }

static char peek(const Lexer *l, size_t offset) {
    if (l->i + offset >= l->buf_len) {
        return '\0';
    }
    return l->buffer[l->i + offset];
}

static void consume(Lexer *l, size_t count) {
    for (size_t k = 0; k < count && l->i < l->buf_len; ++k) {
        if (l->buffer[l->i] == '\n') {
            ++l->ln;
            l->cn = 1;
        } else {
            ++l->cn;
        }
        ++l->i;
    }
}

static size_t consume_while(Lexer *l, bool (*pred)(char)) {
    size_t count = 0;
    while (!is_at_end(l) && pred(peek(l, 0))) {
        consume(l, 1);
        ++count;
    }
    return count;
}

static Mark mark(const Lexer *l) { return (Mark){.offset = l->i, .ln = l->ln, .cn = l->cn}; }

static void error(const Lexer *l, Mark at, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s:%zu:%zu: ", l->file, at.ln, at.cn);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    had_error = true;
}

static char *substr(Lexer *l, size_t start, size_t end) {
    const size_t length = end - start;
    char *out = arena_alloc(l->str_arena, length + 1);
    memcpy(out, l->buffer + start, length);
    out[length] = '\0';
    return out;
}

static void emit(Lexer *l, Mark start, TokenKind kind, const char *lexeme, size_t int_lit) {
    Token *token = arena_alloc(l->tokens, sizeof(Token));
    *token = (Token){.kind = kind,
                     .lexeme = lexeme,
                     .int_lit = int_lit,
                     .position = (Position){.ln = start.ln, .cn = start.cn},
                     .next = NULL};
    l->t_token->next = token;
    l->t_token = token;
}

static void skip_block_comment(Lexer *l, Mark start) {
    consume(l, 2);
    while (!is_at_end(l)) {
        if (peek(l, 0) == '*' && peek(l, 1) == '/') {
            consume(l, 2);
            return;
        }
        consume(l, 1);
    }
    error(l, start, "Unterminated block comment.");
}

static void lex_identifier(Lexer *l, Mark start) {
    consume_while(l, is_ident_char);
    const size_t length = l->i - start.offset;
    char *word = substr(l, start.offset, l->i);
    const struct Keyword *keyword = get_keyword_kind(word, length);
    emit(l, start, keyword != NULL ? keyword->token_kind : IDENTIFIER, word, 0);
}

static void lex_radix_digits(Lexer *l, Mark start, bool (*is_valid)(char),
                             const char *empty_msg) {
    consume(l, 2);
    if (consume_while(l, is_valid) == 0) {
        error(l, start, "%s", empty_msg);
    }
}

static void lex_octal_digits(Lexer *l) {
    consume(l, 1);
    while (is_digit(peek(l, 0))) {
        const char c = peek(l, 0);
        if (!is_octal(c)) {
            error(l, mark(l), "Invalid digit '%c' in octal constant.", c);
        }
        consume(l, 1);
    }
}

static unsigned long long parse_int(const char *text) {
    if (text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        return strtoull(text + 2, NULL, 2);
    }
    return strtoull(text, NULL, 0);
}

static void lex_number(Lexer *l, Mark start) {
    const char next = peek(l, 1);

    if (peek(l, 0) == '0' && (next == 'x' || next == 'X')) {
        lex_radix_digits(l, start, is_xdigit, "Hex constant has no digits.");
    } else if (peek(l, 0) == '0' && (next == 'b' || next == 'B')) {
        lex_radix_digits(l, start, is_bin_digit, "Binary constant has no digits.");
    } else if (peek(l, 0) == '0') {
        lex_octal_digits(l);
    } else {
        consume_while(l, is_digit);
    }

    if (is_ident_char(peek(l, 0))) {
        error(l, mark(l), "Invalid character in numeric constant.");
        consume_while(l, is_ident_char);
    }

    char *text = substr(l, start.offset, l->i);
    emit(l, start, INT, text, (size_t)parse_int(text));
}

static size_t decode_escapes(Lexer *l, Mark start, char *s, size_t len) {
    size_t r = 0, w = 0;
    while (r < len) {
        if (s[r] != '*') {
            s[w++] = s[r++];
            continue;
        }
        ++r;
        switch (s[r]) {
        case '0': s[w++] = '\0'; break;
        case 'e': s[w++] = 0x04; break;
        case '(': s[w++] = '{'; break;
        case ')': s[w++] = '}'; break;
        case 't': s[w++] = '\t'; break;
        case 'n': s[w++] = '\n'; break;
        case '"': s[w++] = '"'; break;
        case '\'': s[w++] = '\''; break;
        case '*': s[w++] = '*'; break;
        default: error(l, start, "Unknown escape sequence `*%c`", s[r]); break;
        }
        ++r;
    }
    s[w] = '\0';
    return w;
}

static bool consume_until_quote(Lexer *l, char quote) {
    while (!is_at_end(l) && peek(l, 0) != '\n') {
        const char c = peek(l, 0);
        if (c == quote) {
            return true;
        }
        if (c == '*') {
            consume(l, 1);
            if (is_at_end(l) || peek(l, 0) == '\n') {
                return false;
            }
        }
        consume(l, 1);
    }
    return false;
}

static void lex_quoted(Lexer *l, Mark start) {
    const char quote = peek(l, 0);
    consume(l, 1);
    const size_t body = l->i;

    if (!consume_until_quote(l, quote)) {
        error(l, start,
              quote == '"' ? "Unterminated string literal." : "Unterminated character constant.");
        return;
    }

    char *text = substr(l, body, l->i);
    const size_t length = decode_escapes(l, start, text, l->i - body);
    consume(l, 1);

    if (quote == '"') {
        emit(l, start, STRING, text, ++l->srt_data_c);
        return;
    }

    if (length > 2) {
        error(l, start, "Character constant too long.");
    }
    emit(l, start, INT, text, (size_t)text[0]);
}

static bool lex_punctuator(Lexer *l, Mark start) {
    const size_t count = sizeof(punctuators) / sizeof(punctuators[0]);
    for (size_t k = 0; k < count; ++k) {
        const Punctuator *p = &punctuators[k];
        if (l->i + p->len > l->buf_len || memcmp(l->buffer + l->i, p->text, p->len) != 0) {
            continue;
        }
        consume(l, p->len);
        emit(l, start, p->kind, NULL, 0);
        return true;
    }
    return false;
}

static void scan_token(Lexer *l) {
    const Mark start = mark(l);
    const char c = peek(l, 0);
    l->t_cn = start.cn;

    if (is_space(c)) {
        consume(l, 1);
        return;
    }
    if (c == '/' && peek(l, 1) == '*') {
        skip_block_comment(l, start);
        return;
    }
    if (is_alpha(c) || c == '_') {
        lex_identifier(l, start);
        return;
    }
    if (is_digit(c)) {
        lex_number(l, start);
        return;
    }
    if (c == '"' || c == '\'') {
        lex_quoted(l, start);
        return;
    }
    if (lex_punctuator(l, start)) {
        return;
    }

    error(l, start, "Unexpected character '%c'.", c);
    consume(l, 1);
}

Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len) {
    Lexer *l = malloc(sizeof(Lexer));
    l->file = file;
    l->buffer = buffer;
    l->buf_len = buf_len;
    l->i = 0;
    l->ln = 1;
    l->cn = 1;
    l->t_cn = 1;
    l->srt_data_c = 0;
    l->tokens = init_arena(sizeof(Token) * TOKENS_STORE);
    l->str_arena = init_arena(sizeof(char) * (buf_len * 0.75));
    l->t_token = NULL;
    l->tok_head = NULL;
    return l;
}

void lexer(Lexer *l) {
    had_error = false;

    l->tok_head = arena_alloc(l->tokens, sizeof(Token));
    l->tok_head->next = NULL;
    l->t_token = l->tok_head;

    while (!is_at_end(l)) {
        scan_token(l);
    }

    if (had_error) {
        exit(EXIT_FAILURE);
    }
}

void free_lexer(Lexer *l) {
    free_arena(l->tokens);
    free_arena(l->str_arena);
    free(l);
}

Position position(size_t ln, size_t cn) { return (Position){.ln = ln, .cn = cn}; }

char *token_kind_to_str(TokenKind kind) {
    switch (kind) {
#define TOKEN(tok)                                                                               \
    case tok: return #tok;
#define KEYWORDS(tok, name)                                                                      \
    case tok: return name;
#define PUNCTUATION(tok, name)                                                                   \
    case tok: return name;
#include "token.def"
#undef TOKEN
#undef KEYWORDS
#undef PUNCTUATION
    default: return "nil";
    }
}
