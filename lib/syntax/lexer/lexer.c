#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/syntax/lexer/process.h"

Lexer *init_lexer(const char *file_path, const char *buffer, size_t buf_len) {
    Lexer *l = malloc(sizeof(Lexer));
    l->file_path = file_path;
    l->buffer = buffer;
    l->buf_len = buf_len;
    l->i = 0;
    l->ln = 1;
    l->cn = 1;
    l->t_cn = 1;
    l->srt_data_c = 0;
    l->tokens = init_arena(sizeof(Token) * TOKENS_STORE);
    l->locations = init_arena(sizeof(Location) * TOKENS_STORE);
    l->lexeme = init_arena(sizeof(char) * (buf_len * 0.75));
    l->t_token = NULL;
    l->tok_head = NULL;
    return l;
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

Token *lexer_lex(Lexer *l) {
    l->had_error = false;

    l->tok_head = arena_alloc(l->tokens, sizeof(Token));
    l->tok_head->next = NULL;
    l->t_token = l->tok_head;

    while (!is_at_end(l)) {
        scan_token(l);
    }

    if (l->had_error) {
        exit(EXIT_FAILURE);
    }
    return l->tok_head->next;
}

void free_lexer(Lexer *l) {
    free_arena(l->tokens);
    free_arena(l->lexeme);
    free_arena(l->locations);
    free(l);
}
