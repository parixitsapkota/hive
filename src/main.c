#include <stdio.h>
#include <stdlib.h>

#include "include/core/file.h"
#include "include/syntax/lexer/lexer.h"

int main(const int argc, const char *const *argv) {
    if (argc != 2) {
        fprintf(stderr, "%s : incorrect usage.\n", argv[0]);
        fprintf(stderr, "Usage : %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *file_name = argv[1];

    FILE *file = fopen(file_name, "rb");
    size_t buf_len = 0;
    const char *buffer = readf(file, &buf_len);

    Lexer *lexer_context = init_lexer(file_name, buffer, buf_len);

    Token *cur_tok = lexer_lex(lexer_context);
    while (cur_tok != NULL) {
        fprintf(stderr, "%s:%zu:%zu:\t %-11s : %s.\n", file_name, cur_tok->location->ln,
                cur_tok->location->cn, token_kind_to_str(cur_tok->kind), cur_tok->lexeme);
        cur_tok = cur_tok->next;
    }

    free_lexer(lexer_context);

    return 0;
}
