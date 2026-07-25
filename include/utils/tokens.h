#pragma once

#include <utils/literal.h>

#define KEYWORDS \
  X(TOKEN_IMPORT, "import") \
  X(TOKEN_PUBLIC, "public") \
  X(TOKEN_CONST, "const") \
  X(TOKEN_VAR, "var") \
  X(TOKEN_SET, "set") \
  X(TOKEN_IF, "if") \
  X(TOKEN_ELSE, "else") \
  X(TOKEN_DO, "do") \
  X(TOKEN_WHILE, "while") \
  X(TOKEN_FOR, "for") \
  X(TOKEN_RETURN, "return") \
  X(TOKEN_CONTINUE, "continue") \
  X(TOKEN_BREAK, "break") \
  X(TOKEN_ENUM, "enum") \
  X(TOKEN_STRUCT, "struct") \
  X(TOKEN_THIS, "this") \
  X(TOKEN_AS, "as")

#define ASSIGNMENTS \
  X(TOKEN_ASSIGN, "=") \
  X(TOKEN_PLUS_ASSIGN, "+=") \
  X(TOKEN_MINUS_ASSIGN, "-=") \
  X(TOKEN_ASTERISK_ASSIGN, "*=") \
  X(TOKEN_SLASH_ASSIGN, "/=") \
  X(TOKEN_MOD_ASSIGN, "%=") \
  X(TOKEN_BIT_OR_ASSIGN, "|=") \
  X(TOKEN_BIT_XOR_ASSIGN, "^=") \
  X(TOKEN_BIT_AND_ASSIGN, "&=") \
  X(TOKEN_SHIFT_LEFT_ASSIGN, "<<=") \
  X(TOKEN_SHIFT_RIGHT_ASSIGN, ">>=")

#define OPERATORS \
  X(TOKEN_PLUS, "+") \
  X(TOKEN_MINUS, "-") \
  X(TOKEN_ASTERISK, "*") \
  X(TOKEN_SLASH, "/") \
  X(TOKEN_MOD, "%") \
  X(TOKEN_NOT, "!") \
  X(TOKEN_EQ, "==") \
  X(TOKEN_NE, "!=") \
  X(TOKEN_GT, ">") \
  X(TOKEN_GE, ">=") \
  X(TOKEN_LT, "<") \
  X(TOKEN_LE, "<=") \
  X(TOKEN_BIT_NOT, "~") \
  X(TOKEN_BIT_OR, "|") \
  X(TOKEN_BIT_XOR, "^") \
  X(TOKEN_BIT_AND, "&") \
  X(TOKEN_SHIFT_LEFT, "<<") \
  X(TOKEN_SHIFT_RIGHT, ">>") \
  X(TOKEN_OR, "||") \
  X(TOKEN_AND, "&&") \
  X(TOKEN_INCREMENT, "++") \
  X(TOKEN_DECREMENT, "--")

#define SYMBOLS \
  X(TOKEN_COMMA, ",") \
  X(TOKEN_COLON, ":") \
  X(TOKEN_SEMICOLON, ";") \
  X(TOKEN_DOT, ".") \
  X(TOKEN_ARROW, "=>") \
  X(TOKEN_LPAREN, "(") \
  X(TOKEN_RPAREN, ")") \
  X(TOKEN_LBRACE, "{") \
  X(TOKEN_RBRACE, "}") \
  X(TOKEN_LBRACKET, "[") \
  X(TOKEN_RBRACKET, "]") \

#define OTHERS \
  X(LITERAL_INTEGER, "literal") \
  X(LITERAL_UINTEGER, "literal") \
  X(LITERAL_FLOAT, "literal") \
  X(LITERAL_DOUBLE, "literal") \
  X(LITERAL_CHAR, "literal") \
  X(LITERAL_STRING, "literal") \
  X(LITERAL_BOOLEAN, "literal") \
  X(IDENTIFIER, "identifier")

enum TokenType {
#define X(name, str) name,
  KEYWORDS
  ASSIGNMENTS
  OPERATORS
  SYMBOLS
  OTHERS
#undef X

  TEMPLATE_START,
  TEMPLATE_END,
  TOKEN_EOF
};

static const char *tk_names[] = {
#define X(name, str) [name] = str,
  KEYWORDS
  ASSIGNMENTS
  OPERATORS
  SYMBOLS
  OTHERS
#undef X
};

struct Token {
  enum TokenType type;
  union Literal literal;
  int line;
  int column;
};
