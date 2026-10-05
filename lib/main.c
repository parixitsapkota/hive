#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/core/file.h"
#include "include/core/info.h"
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

    Token *tokens = lexer_lex(lexer_context);

    for (const Token *cur = tokens; cur != NULL; cur = cur->next) {
        fprintf(stderr,
                FG_CYAN "%s" RESET ":" FG_BLACK "%zu:%zu:%zu:%zu"
                        "\t" BOLD FG_YELLOW "%-12s " FG_RED "%s" RESET "\n",
                file_name, cur->span.start->ln, cur->span.start->cn, cur->span.end->ln,
                cur->span.end->cn, token_kind_to_str(cur->kind),
                cur->lexeme ? cur->lexeme : token_kind_to_str(cur->kind));
    }

    free_lexer(lexer_context);

    return 0;
}
