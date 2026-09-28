
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dep/keywords.h"
#include "lexer.h"

// Lexer helper funcions
char peak(const Lexer *l, int offset);
void consume(Lexer *l);
void add_token(Lexer *l, TokenKind kind, const char *lexeme, size_t int_lit, Position position);
Position position(size_t ln, size_t cn);
// P_Print helper function
char *token_kind_to_str(TokenKind kind);
// Lexer core functions
char *substr(Lexer *l, const char *buffer, const size_t start, const size_t end);
char *get_word(Lexer *l);
char *get_string_ident(Lexer *l);
char *get_string(Lexer *l, char q);
char *get_digit(Lexer *l);

Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len) {
  Lexer *l = malloc(sizeof(Lexer));
  l->file = file;
  l->buffer = buffer;
  l->buf_len = buf_len;
  l->i = 0;
  l->ln = 1;
  l->cn = 1;
  l->srt_data_c = 0;
  l->tokens = init_arena(sizeof(Token) * TOKENS_STORE);
  l->str_arena = init_arena(sizeof(char) * (buf_len * 0.75));
  // Temp vars.
  l->t_token = NULL;
  l->tok_head = NULL;
  return l;
}

void lexer(Lexer *l) {
  l->tok_head = arena_alloc(l->tokens, sizeof(Token));
  l->tok_head->next = NULL;
  l->t_token = l->tok_head;

  while (l->i < l->buf_len) {
    char c = peak(l, 0);
    l->t_cn = l->cn;

    // Count lines.
    if (isspace(c)) {
      if (c == '\n') {
        l->cn = 0;
        ++l->ln;
      } else {
        ++l->cn;
      }
      consume(l);
      continue;
    }

    // Collect Identifiers.
    if (isalpha(c) || c == '_') {
      size_t start = l->i;
      char *word = get_word(l);
      size_t word_len = l->i - start;

      const struct Keyword *k = get_keyword_kind(word, word_len);
      if (k != NULL) {
        add_token(l, k->token_kind, word, 0, position(l->ln, l->t_cn));
      } else {
        add_token(l, IDENTIFIER, word, 0, position(l->ln, l->t_cn));
      }
      continue;
    }

    // Handle digits.
    if (isdigit((unsigned char)c)) {
      char *num = get_digit(l);
      char *endptr;
      unsigned long long int_lit = strtoull(num, &endptr, 0);
      if ((num[0] == '0') && (num[1] == 'b' || num[1] == 'B')) {
        int_lit = strtoull(num + 2, &endptr, 2);
      }
      add_token(l, INT, num, (size_t)int_lit, position(l->ln, l->t_cn));
      continue;
    }

    // Collect Str literal
    if (c == '"') {
      char *str = get_string(l, '"');
      add_token(l, STRING, str, ++(l->srt_data_c), position(l->ln, l->t_cn));
      continue;
    }

    // Collect Char literal
    if (c == '\'') {
      char *str = get_string(l, '\'');
      add_token(l, INT, str, (size_t)str[0], position(l->ln, l->t_cn));
      continue;
    }

    // Skip Comments.
    if (c == '/' && peak(l, 1) == '*') {
      while (l->i < l->buf_len) {
        if (peak(l, 0) == '*' && peak(l, 1) == '/') {
          consume(l);
          consume(l);
          break;
        }
        if (peak(l, 0) == '\n') {
          l->cn = 0;
          ++l->ln;
        }
        consume(l);
      }
      continue;
    }

#define CASE_1(a, out_a)                                                                         \
  case a: kind = out_a; break

#define CASE_2(a, b, out_a, out_a_b)                                                             \
  case a:                                                                                        \
    if (peak(l, 1) == (b)) {                                                                     \
      kind = (out_a_b);                                                                          \
      consume(l);                                                                                \
    } else {                                                                                     \
      kind = (out_a);                                                                            \
    }                                                                                            \
    break

    {
      TokenKind kind;

      switch (c) {
        CASE_1('{', O_BRACE);
        CASE_1('}', C_BRACE);
        CASE_1('[', O_BRACKET);
        CASE_1(']', C_BRACKET);
        CASE_1('(', O_PREN);
        CASE_1(')', C_PREN);
        CASE_1(';', SEMICOLON);
        CASE_1(':', COLON);
        CASE_1('?', Q_MARK);
        CASE_1('&', BIT_AND);
        CASE_1('|', BIT_OR);

        CASE_1(',', COMMA);
        CASE_1('*', MUL);
        CASE_1('/', DEV);
        CASE_1('%', MOD);

        CASE_2('+', '+', ADD, INC);
        CASE_2('-', '-', SUB, DEC);
        CASE_2('!', '=', NOT, NOT_EQUAL);
        CASE_2('=', '=', ASSIGN, EQUAL);

      case '<':
        if (peak(l, 1) == '=') {
          kind = LESSER_EQUAL;
          consume(l);
        } else if (peak(l, 1) == '<') {
          kind = BITSHIFT_L;
          consume(l);
        } else {
          kind = LESSER;
        }
        break;

      case '>':
        if (peak(l, 1) == '=') {
          kind = GREATER_EQUAL;
          consume(l);
        } else if (peak(l, 1) == '>') {
          kind = BITSHIFT_R;
          consume(l);
        } else {
          kind = GREATER;
        }
        break;

      default: kind = UNKNOWN; break;
      }

      if (kind == UNKNOWN) {
        add_token(l, UNKNOWN, NULL, 0, position(l->ln, l->t_cn));
        consume(l);
        continue;
      }
      add_token(l, kind, NULL, 0, position(l->ln, l->t_cn));
      consume(l);
      continue;
    }
  }
}

void free_lexer(Lexer *l) {
  free_arena(l->tokens);
  free_arena(l->str_arena);
  free(l);
}

// Lexer helper funcions
char peak(const Lexer *l, const int offset) { return l->buffer[l->i + offset]; }

void consume(Lexer *l) {
  ++l->i;
  ++l->cn;
}

void add_token(Lexer *l, TokenKind kind, const char *lexeme, size_t int_lit, Position position) {
  Token *new_token = arena_alloc(l->tokens, sizeof(Token));
  *new_token = (Token){
      .kind = kind, .lexeme = lexeme, .int_lit = int_lit, .position = position, .next = NULL};
  new_token->next = NULL;
  l->t_token->next = new_token;
  l->t_token = new_token;
}

Position position(size_t ln, size_t cn) { return (Position){.ln = ln, .cn = cn}; }

// Lexer core functions
char *substr(Lexer *l, const char *buffer, const size_t start, const size_t end) {
  const size_t length = end - start;
  char *substr = arena_alloc(l->str_arena, length + 1);
  strncpy(substr, buffer + start, length);
  substr[length] = '\0';
  return substr;
}

char *get_word(Lexer *l) {
  const size_t start = l->i;
  while (isalnum(peak(l, 0)) || peak(l, 0) == '_') {
    consume(l);
  }
  return substr(l, l->buffer, start, l->i);
}

char *get_string(Lexer *l, char q) {
  consume(l); // Consume opening " / '
  const size_t start = l->i;

  while (peak(l, 0) != q) {
    char c = peak(l, 0);
    if (c == '\0' || c == '\n') {
      fprintf(stderr, "%s:%zu:%zu: Unterminated quoted constant literal.\n", l->file, l->ln,
              l->t_cn);
      exit(EXIT_FAILURE);
    }
    if (c == '*') {
      consume(l); // Consume prefix '*'
      c = peak(l, 0);
      if (c == '\0' || c == '\n') {
        fprintf(stderr, "%s:%zu:%zu: Unterminated quoted constant after escape.\n", l->file,
                l->ln, l->t_cn);
        exit(EXIT_FAILURE);
      }
    }
    consume(l);
  }

  char *string = substr(l, l->buffer, start, l->i);
  consume(l); // Consume closing " / '

  size_t strlen = l->i - start;
  size_t r = 0, w = 0;
  while (r < strlen) {
    if (string[r] == '*') {
      r++;
      switch (string[r]) {
      case '0': string[w++] = '\0'; break;
      case 'e': string[w++] = 0x04; break;
      case '(': string[w++] = '{'; break;
      case ')': string[w++] = '}'; break;
      case 't': string[w++] = '\t'; break;
      case '"': string[w++] = '\"'; break;
      case 'n': string[w++] = '\n'; break;
      case '*': string[w++] = '*'; break;
      case '\'': string[w++] = '\''; break;
      default:
        fprintf(stderr, "%s:%zu:%zu: Unknown escape sequence `*%c`\n", l->file, l->ln, l->t_cn,
                string[r]);
        exit(EXIT_FAILURE);
      }
    } else {
      string[w++] = string[r];
    }
    r++;
  }

  if (q == '\'' && w - 1 > 1) {
    fprintf(stderr, "%s:%zu:%zu: Character constant too long.\n", l->file, l->ln, l->t_cn);
  }

  string[w] = '\0';
  return string;
}

bool isoctal(char c) {
  if ((c >= 0x30 && c <= 0x37)) {
    return true;
  }
  return false;
}

char *get_digit(Lexer *l) {
  const size_t start = l->i;

  if (peak(l, 0) == '0') {
    char next = peak(l, 1);
    consume(l);

    if (next == 'x' || next == 'X') {
      consume(l);
      while (isxdigit(peak(l, 0))) {
        consume(l);
      }
      return substr(l, l->buffer, start, l->i);
    }

    if (next == 'b' || next == 'B') {
      consume(l);
      while (peak(l, 0) == '0' || peak(l, 0) == '1') {
        consume(l);
      }
      return substr(l, l->buffer, start, l->i);
    }

    while (isdigit(peak(l, 0))) {
      char c = peak(l, 0);
      if (!isoctal(c)) {
        fprintf(stderr, "%s:%zu:%zu: Invalid digit '%c' in octal constant.\n", l->file, l->ln,
                l->t_cn, c);
      }
      consume(l);
    }
    return substr(l, l->buffer, start, l->i);
  }

  while (isdigit(peak(l, 0))) {
    consume(l);
  }
  return substr(l, l->buffer, start, l->i);
}

char *token_kind_to_str(TokenKind kind) {
  switch (kind) {
  case UNKNOWN: return "UNKNOWN";
  case END_OF_TOKEN: return "END_OF_TOKEN";
  case IDENTIFIER: return "IDENTIFIER";
  case INT: return "INT";
  case STRING: return "STRING";
  case LABLE: return "LABLE";
  case EXTRN: return "EXTRN";
  case AUTO: return "AUTO";
  case IF: return "IF";
  case ELSE: return "ELSE";
  case WHILE: return "WHILE";
  case GOTO: return "GOTO";
  case RETURN: return "RETURN";
  case O_BRACE: return "{";
  case C_BRACE: return "}";
  case O_PREN: return "(";
  case C_PREN: return ")";
  case O_BRACKET: return "[";
  case C_BRACKET: return "]";
  case SEMICOLON: return ";";
  case COLON: return ":";
  case Q_MARK: return "?";
  case NOT: return "!";
  case COMMA: return ",";
  case ADD: return "+";
  case SUB: return "-";
  case MUL: return "*";
  case DEV: return "/";
  case MOD: return "%";
  case ASSIGN: return "=";
  case INC: return "++";
  case DEC: return "--";
  case BIT_AND: return "&";
  case BIT_OR: return "|";
  case BITSHIFT_L: return "<<";
  case BITSHIFT_R: return ">>";
  case EQUAL: return "==";
  case NOT_EQUAL: return "!=";
  case LESSER: return "<";
  case GREATER: return ">";
  case LESSER_EQUAL: return "<=";
  case GREATER_EQUAL: return ">=";

  // Default
  default: return "nil";
  }
}
