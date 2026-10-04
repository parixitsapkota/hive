#ifndef _TRIDENT_PARSER_H_
#define _TRIDENT_PARSER_H_

#include <stdbool.h>

#include "ast.h"
#include "lexer.h"

// Parser Structure
typedef struct {
    Lexer *l;
    // Position
    size_t i; // index
    // Ast store
    Arena *ast;
    Arena *var_info;
    AstNode *ast_head;
    AstScope *global_scope_n;
    Hs *global_table;
    Hs *functions_table;
    // Helper/Temp vars
    Hs *t_lable_tab;
    Token *tok;
    AstNode *t_node;
} Parser;

/// Returns a parser context based on given lexing context.
Parser *init_parser(Lexer *l);
/// parses based on the given lexer context and mutates the parser state accordingly.
void parser(Parser *p);
/// Frees the allocated memory in the Parsing context.
void free_parser(Parser *p);

typedef enum {
    PREC_UNKNOWN = -1,
    PREC_NONE = 0,
    PREC_ASSIGNMENT,
    PREC_CONDITIONAL,
    PREC_BIT_OR,
    PREC_BIT_AND,
    PREC_RELATIONAL,
    PREC_EQUALITY,
    PREC_BITSHIFT,
    PREC_ADDITIVE,
    PREC_MULTIPLICATIVE,
} Precedence;

Token *ppeak(const Parser *p);
Token *curr(const Parser *p);
Token *pconsume(Parser *p);
void add_node(AstNode **t_node, AstNode *node);
void expect_and_consume(Parser *p, TokenKind kind);
bool is_kind(Parser *p, TokenKind kind);

AstAtom *parse_atom_f(Parser *p);
AstNode *parse_expr_f(Parser *p, Precedence prec);

#endif // _TRIDENT_PARSER_H_
