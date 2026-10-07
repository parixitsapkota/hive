#include <stdio.h>
#include <stdlib.h>

#include "include/core/source.h"
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
    Source *src = init_source(file_name);
    Lexer *lexer_context = init_lexer(src);
    Token *tokens = lexer_lex(lexer_context);
    Parser *parser_context = init_parser(src, tokens);
    AstNode *root_node = parser_parse(parser_context);
    print_ast(root_node);
    free_parser(parser_context);
    free_lexer(lexer_context);
    free_source(src);

    return 0;
}
