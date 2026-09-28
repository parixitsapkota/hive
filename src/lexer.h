#ifndef _TRIDENT_LEXER_H_
#define _TRIDENT_LEXER_H_

#include <stddef.h>

#include "dep/shi_arena.h"

// Token Kinds
typedef enum {
  // Misc Tokens
  UNKNOWN = 0,
  END_OF_TOKEN,

  // Identifier & Literals
  IDENTIFIER,
  INT,
  STRING,
  LABLE,

  // Keywords
  EXTRN,
  AUTO,
  IF,
  ELSE,
  WHILE,
  GOTO,
  RETURN,

  // Seperator
  O_BRACE,   // `{`
  C_BRACE,   // `}`
  O_PREN,    // `(`
  C_PREN,    // `)`
  O_BRACKET, // `[`
  C_BRACKET, // `]`
  SEMICOLON, // `;`
  COLON,     // `:`
  Q_MARK,    // `?`

  // Operator
  COMMA, // `,`
  NOT,   // `!`
  ADD,   // `+`
  SUB,   // `-`
  MUL,   // `*`
  DEV,   // `/`
  MOD,   // `%`

  BIT_AND, // `&`
  BIT_OR,  // `|`

  BITSHIFT_L, // `<<`
  BITSHIFT_R, // `>>`

  EQUAL,         // `==`
  NOT_EQUAL,     // `!=`
  GREATER,       // `>`
  GREATER_EQUAL, // `>=`
  LESSER,        // `<`
  LESSER_EQUAL,  // `<=`

  ASSIGN, // `=`

  INC, // `++`
  DEC, // `--`
} TokenKind;

typedef struct {
  // Position
  size_t ln; // line number;
  size_t cn; // comume number;
} Position;

// Token Defination
typedef struct Token {
  // Value
  TokenKind kind;
  const char *lexeme;
  size_t int_lit;
  // Position
  Position position;
  // Next Token
  struct Token *next;
} Token;

/// Returns a tokenKind string based on given tokenKind.
char *token_kind_to_str(TokenKind kind);
Position position(size_t ln, size_t cn);

// Lexer Structure
typedef struct {
  // buffer file name
  const char *file;
  // Input buffer
  const char *buffer;
  size_t buf_len;
  // Position
  size_t i;  // index
  size_t ln; // line number
  size_t cn; // colume number
  // String storage.
  Arena *str_arena;
  size_t srt_data_c;
  // Token List
  Arena *tokens;
  Token *tok_head;
  // Helper/Temp vars
  Token *t_token;
  size_t t_cn;
} Lexer;

#define TOKENS_STORE 1024

/// Returns a lexer context based on given buffer and length of the buffer.
Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len);
/// Lexes based on the given lexer context and mutates the state accordingly.
void lexer(Lexer *l);
/// Frees the allocated memory in the lexing context.
void free_lexer(Lexer *l);

#endif // _TRIDENT_LEXER_H_
