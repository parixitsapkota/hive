#include <llvm-c/Core.h>
#include <stdio.h>

#include "include/cgen/cgen.h"
#include "include/core/diags.h"
#include "include/core/source.h"
#include "include/sema/sema.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/lexer/lexer.h"
#include "include/syntax/parser/parser.h"

int main(const int argc, const char *const *argv) {
    if (argc != 3)
        fatal("incorrect usage! \nCorrect usage : %s <file_pat> <output>", argv[0]);

    const char *file_name = argv[1];
    Source *src = init_source(file_name);
    Lexer *lexer_context = init_lexer(src);
    Token *tokens = lexer_lex(lexer_context);
    Parser *parser_context = init_parser(src, tokens);
    AstNode *root_node = parser_parse(parser_context);

    print_ast(root_node);

    Sema *sema_context = init_sema(src, root_node);
    sema(sema_context);

    Cgen *cgen_context = init_cgen(src, root_node);
    cgen(cgen_context);
    cgen_emit_object(cgen_context, argv[2]);

    free_sema(sema_context);

    free_parser(parser_context);
    free_lexer(lexer_context);
    free_source(src);

    return 0;
}
