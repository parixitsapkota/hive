#include <stdio.h>

#include "include/core/info.h"
#include "include/syntax/ast/ast.h"
#include "include/syntax/ast/visitor.h"
#include "include/syntax/lexer/tokens.h"

static const char *ast_kind_to_string(AstKind kind) {
    static const char *const names[] = {
        /* expressions */
        [AST_INT] = "AST_INT",
        [AST_CHAR] = "AST_CHAR",
        [AST_STRING] = "AST_STRING",
        [AST_IDENT] = "AST_IDENT",
        [AST_UNARY] = "AST_UNARY",
        [AST_BINARY] = "AST_BINARY",
        [AST_ASSIGN] = "AST_ASSIGN",
        [AST_TERNARY] = "AST_TERNARY",
        [AST_INDEX] = "AST_INDEX",
        [AST_CALL] = "AST_CALL",

        /* statements */
        [AST_BLOCK] = "AST_BLOCK",
        [AST_EXPR_STMT] = "AST_EXPR_STMT",
        [AST_NULL_STMT] = "AST_NULL_STMT",
        [AST_IF] = "AST_IF",
        [AST_WHILE] = "AST_WHILE",
        [AST_SWITCH] = "AST_SWITCH",
        [AST_CASE] = "AST_CASE",
        [AST_BREAK] = "AST_BREAK",
        [AST_RETURN] = "AST_RETURN",
        [AST_GOTO] = "AST_GOTO",
        [AST_LABEL] = "AST_LABEL",

        /* declarations */
        [AST_AUTO] = "AST_AUTO",
        [AST_EXTRN] = "AST_EXTRN",
        [AST_GLOBAL_DECL] = "AST_GLOBAL_DECL",
        [AST_FUNCTION] = "AST_FUNCTION",
    };

    size_t count = sizeof(names) / sizeof(names[0]);
    if ((size_t)kind < count && names[kind] != NULL) {
        return names[kind];
    }
    return "AST_UNKNOWN";
}

static VisitAction ast_printer(AstNode *node, const VisitContext *ctx) {
    for (size_t i = 0; i < ctx->depth; i++) {
        printf("   ");
    }

    switch (node->kind) {
    case AST_INT:
        printf(FG_BLACK "%s: " FG_GREEN "\"%zu\"\n" RESET, ast_kind_to_string(node->kind),
               node->as.int_val);
        break;
    case AST_IDENT:
        printf(FG_BLACK "%s: " FG_GREEN "\"%s\"\n" RESET, ast_kind_to_string(node->kind),
               node->as.ident.name);
        break;
    case AST_STRING:
        printf(FG_BLACK "%s: " FG_GREEN "\"%s\"\n" RESET, ast_kind_to_string(node->kind),
               node->as.string.str);
        break;
    case AST_BINARY:
        printf(FG_BLACK "%s: " FG_YELLOW "op" FG_BLACK " : " FG_RED "`%s`\n" RESET,
               ast_kind_to_string(node->kind), token_kind_to_str(node->as.binary.op));
        break;
    case AST_FUNCTION:
        printf(FG_BLACK "%s: " FG_GREEN "\"%s\"\n" RESET, ast_kind_to_string(node->kind),
               node->as.func.name);
        break;
    default: printf(FG_RED "%s\n" RESET, ast_kind_to_string(node->kind)); break;
    }

    return VISIT_CONTINUE;
}

void print_ast(AstNode *root) { ast_visit(root, ast_printer, NULL); }
