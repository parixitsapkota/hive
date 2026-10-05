#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/core/arena.h"
#include "include/core/file.h"
#include "include/core/info.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/lexer.h"
#include "include/syntax/lexer/tokens.h"

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

    Arena *a = init_arena(1024 * sizeof(AstNode));

    AstNode *left = ast_int_val(a, NULL, 34);
    AstNode *right = ast_int_val(a, NULL, 35);
    AstNode *expr = ast_binary(a, NULL, AST_BINARY, TOK_ADD, left, right);
    AstNode *ret = ast_stmt(a, NULL, AST_RETURN, expr);
    AstNode *body = ast_block(a, NULL, ret);
    AstNode *root = ast_function(a, NULL, "func", NULL, 0, body);

    print_ast(root);

    free_arena(a);

    return 0;
}
