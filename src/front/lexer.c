#include <front/lexer.h>
#include <stdlib.h>
#include <utils/error.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

void addToken(struct Tokens *tokens, struct Arena *arena, enum TokenType t, union Literal v, int l, int c) {
  if (tokens->length == tokens->capacity) {
    size_t oldCap = tokens->capacity;
    tokens->capacity *= 2;
    struct Token *temp = arena_alloc(arena, tokens->capacity * sizeof(struct Token));
    memcpy(temp, tokens->token, oldCap * sizeof(struct Token));
    tokens->token = temp;
  }

  struct Token token;
  token.type = t;
  token.literal = v;
  token.line = l;
  token.column = c;

  tokens->token[tokens->length++] = token;
}

void addTokenString(struct Tokens *tokens, struct Arena *arena, enum TokenType t, const char *v, size_t s, int l, int c) {
  union Literal literal = {
    .string.start = v,
    .string.length = s
  };
  addToken(tokens, arena, t, literal, l, c);
}

void addTokenNum(struct Tokens *tokens, struct Arena *arena, enum TokenType t, const char *v, size_t s, int b, int l, int c) {
  union Literal literal;
  if (t == LITERAL_DOUBLE) literal.numDouble = strtod(v, NULL);

  else if (t == LITERAL_FLOAT) {
    errno = 0;
    literal.numFloat = strtof(v, NULL);

    if (errno == ERANGE) {
      literal.numDouble = strtod(v, NULL);
      t = LITERAL_DOUBLE;
    }
  }

  else if (t == LITERAL_INTEGER) {
    errno = 0;
    literal.numInt = strtoll(v, NULL, b);

    if (errno == ERANGE) {
      literal.numUint = strtoull(v, NULL, b);
      t = LITERAL_UINTEGER;
    }
  }

  else if (t == LITERAL_UINTEGER) literal.numUint = strtoull(v, NULL, b);
  addToken(tokens, arena, t, literal, l, c);
}

void next(int *i, int *l, int *c, char ch) {
  if (ch == '\n') {
    (*l)++;
    *c = 1;
  } else (*c)++;

  (*i)++;
}

#define NEXT() next(&index, &line, &col, code[index])

enum Mode {
  Mode_Normal,
  Mode_Text,
  Mode_Code
} mode = Mode_Normal;

uint8_t isNum(char ch, int base) { 
  if (base == 2) return ch == '0' || ch == '1';
  else if (base == 8) return ch >= '0' && ch <= '7';
  else if (base == 16) return (ch >= '0' && ch <= '9') || (tolower(ch) >= 'a' && tolower(ch) <= 'f');
  return ch >= '0' && ch <= '9';
}

void lexer(const char *filename, const char *code, struct Tokens *tokens, struct Arena *arena) {
  int index = 0, line = 1, col = 1, len = strlen(code);

  while (index < len) {
    if (mode == Mode_Text) {
      int start = index;
      while (code[index] && code[index] != '"') {
        if (code[index] == '{') {
          mode = Mode_Code;
          break;
        }

        if (code[index] == '\\') NEXT();
        NEXT();
      }

      if (mode == Mode_Text && code[index] != '"') {
        errorLang(filename, line, col, "missing terminating '\"' character");
      }

      addTokenString(tokens, arena, LITERAL_STRING, code + start, index - start, line, col - (index - start));
      if (code[index] == '"') {
        mode = Mode_Normal;
        addTokenString(tokens, arena, TEMPLATE_END, "", 0, line, col - (index - start));
      }

      NEXT();
      continue;
    }

    if (mode == Mode_Code && code[index] == '}') {
      mode = Mode_Text;
      NEXT();
      continue;
    }

    if (code[index] == '/' && code[index + 1] == '/') {
      while (code[index] && code[index] != '\n') NEXT();
      continue;
    }

    if (code[index] == '/' && code[index + 1] == '*') {
      NEXT(); NEXT();
      while (code[index] && (code[index] != '*' || code[index + 1] != '/')) NEXT();

      if (code[index] != '*' && code[index + 1] != '/') {
        errorLang(filename, line, col, "unterminated /* comment");
      }

      NEXT(); NEXT();
      continue;
    }

    if (code[index] == '$' && code[index + 1] == '"') {
      mode = Mode_Text;
      addTokenString(tokens, arena, TEMPLATE_START, "", 0, line, col);
      NEXT(); NEXT();
      continue;
    }

    if (code[index] == '"') {
      NEXT();
      int start = index;
      while (code[index] && code[index] != '"') {
        if (code[index] == '\\') NEXT();
        NEXT();
      }

      if (code[index] != '"') {
        errorLang(filename, line, col, "missing terminating '\"' character");
      }

      addTokenString(tokens, arena, LITERAL_STRING, code + start, index - start, line, col - (index - start));
      NEXT();
      continue;
    }

    if (code[index] == '\'') {
      NEXT();
      int start = index;
      uint8_t escape = 0;

      while (code[index] && code[index] != '\'') {
        if (code[index] == '\\') {
          NEXT();
          escape = 1;
        }

        NEXT();
      }

      if (code[index] != '\'') {
        errorLang(filename, line, col, "missing terminating '\'' character");
      }

      if ((!escape && index - start > 1) || (escape && index - start > 2)) {
        errorLang(filename, line, col, "Multi-character character constant");
      }

      addTokenString(tokens, arena, LITERAL_CHAR, code + start, index - start, line, col - (index - start));
      NEXT();
      continue;
    }

    if (code[index] == '_' || (code[index] >= 'a' && code[index] <= 'z') || (code[index] >= 'A' && code[index] <= 'Z')) {
      int start = index;
      while (code[index] == '_' || (code[index] >= 'a' && code[index] <= 'z') || (code[index] >= 'A' && code[index] <= 'Z') ||
          isNum(code[index], 10)) NEXT();

#define X(name, str) \
      if (index - start == strlen(str) && memcmp(code + start, str, index - start) == 0) { \
        addTokenString(tokens, arena, name, code + start, index - start, line, col - (index - start)); \
        continue; \
      }
      KEYWORDS
#undef X

      addTokenString(tokens, arena, IDENTIFIER, code + start, index - start, line, col - (index - start));
      continue;
    }

    if (isNum(code[index], 10)) {
      int start = index;
      enum TokenType type = LITERAL_INTEGER;
      char buffer[30];
      int i = 0;
      int base = 10;

      if (code[index] == '0' && tolower(code[index + 1]) == 'x') {
        base = 16;
        NEXT(); NEXT();
        buffer[i++] = '0';
        buffer[i++] = 'x';
      }

      else if (code[index] == '0' && tolower(code[index + 1]) == 'b') {
        base = 2;
        NEXT(); NEXT();
      }

      else if (code[index] == '0' && tolower(code[index + 1]) == 'o') {
        base = 8;
        NEXT(); NEXT();
        buffer[i++] = '0';
      }

      while (code[index]) {
        if (i == 30) errorLang(filename, line, col, "the number constant too large");
        if (code[index] == '_') NEXT();
        if (type != LITERAL_DOUBLE && code[index] == '.') {
          NEXT();
          buffer[i++] = '.';
          type = LITERAL_DOUBLE;
        }

        if (!isNum(code[index], base)) break;
        buffer[i++] = code[index];
        NEXT();
      }

      if (code[index] == '_') {
        errorLang(filename, line, col, "unexpected '_' character");
      }

      if (code[index] == 'f') {
        NEXT();
        type = LITERAL_FLOAT;
      }

      if (tolower(code[index]) == 'u') {
        NEXT();
        type = LITERAL_UINTEGER;
      }

      buffer[i] = '\0';
      addTokenNum(tokens, arena, type, buffer, i, base, line, col - (index - start));
      continue;
    }

    const char *best = NULL;
    enum TokenType bestType;
    size_t bestLen = 0;

#define X(name, str) \
    if (sizeof(str) - 1 > bestLen && memcmp(code + index, str, sizeof(str) - 1) == 0) { \
      best = str; \
      bestType = name; \
      bestLen = sizeof(str) - 1; \
    }
    ASSIGNMENTS
    OPERATORS
    SYMBOLS
#undef X

    if (best) {
      addTokenString(tokens, arena, bestType, code + index, bestLen, line, col);
      for (int i = 0; i < bestLen; i++) NEXT();
      continue;
    }

    if (isspace(code[index])) {
      NEXT();
      continue;
    }

    errorLang(filename, line, col, "invalid character '%c'", code[index]);
  }

  addTokenString(tokens, arena, TOKEN_EOF, "", 0, line, col);
}

void showTokens(struct Tokens *tokens) {
  printf("=============================== { TOKENS } ===============================\n");
  for (int i = 0; i < tokens->length; i++) {
    printf(" %d:\n", i);
    printf("  type: %d\n", tokens->token[i].type);

    if (tokens->token[i].type == LITERAL_DOUBLE) printf("  value: %f\n", tokens->token[i].literal.numDouble);
    else if (tokens->token[i].type == LITERAL_FLOAT) printf("  value: %f\n", tokens->token[i].literal.numFloat);
    else if (tokens->token[i].type == LITERAL_INTEGER) printf("  value: %ld\n", tokens->token[i].literal.numInt);
    else if (tokens->token[i].type == LITERAL_UINTEGER) printf("  value: %lu\n", tokens->token[i].literal.numUint);
    else printf("  value: %.*s\n", tokens->token[i].literal.string.length, tokens->token[i].literal.string.start);

    printf("  line: %d\n", tokens->token[i].line);
    printf("  column: %d\n", tokens->token[i].column);
  }
  printf("=============================== { TOKENS } ===============================\n");
}
