#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/core/arena.h"
#include "include/core/file.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/lexer.h"
#include "include/syntax/lexer/tokens.h"
#include "include/syntax/parser/parser.h"

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
    Parser *parser_context = init_parser(tokens);
    AstNode *root_node = parser_parse(parser_context);
    print_ast(root_node);
    free_parser(parser_context);
    free_lexer(lexer_context);

    return 0;
}
