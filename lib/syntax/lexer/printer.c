#include <stdio.h>

#include "include/core/info.h"
#include "include/syntax/lexer/tokens.h"

void print_tokens(const char *file_name, const Token *tokens) {
    for (const Token *cur = tokens; cur != NULL; cur = cur->next) {
        fprintf(stderr,
                FG_CYAN "%s" RESET ":" FG_BLACK "%zu:%zu:%zu:%zu"
                        "\t" BOLD FG_YELLOW "%-12s " FG_RED "%s" RESET "\n",
                file_name, cur->span.start->ln, cur->span.start->cn, cur->span.end->ln,
                cur->span.end->cn, token_kind_to_str(cur->kind),
                cur->lexeme ? cur->lexeme : token_kind_to_str(cur->kind));
    }
}
