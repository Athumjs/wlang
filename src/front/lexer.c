#include <front/lexer.h>
#include <utils/error.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void addToken(struct Tokens *tokens, struct Arena *arena, enum TokenType t, const char *v, size_t s, int l, int c) {
  if (tokens->length == tokens->capacity) {
    size_t oldCap = tokens->capacity;
    tokens->capacity *= 2;
    struct Token *temp = arena_alloc(arena, tokens->capacity * sizeof(struct Token));
    memcpy(temp, tokens->token, oldCap * sizeof(struct Token));
    tokens->token = temp;
  }

  struct Token token;
  token.type = t;
  token.string.start = v; 
  token.string.length = s;
  token.line = l;
  token.column = c;

  tokens->token[tokens->length++] = token;
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

      addToken(tokens, arena, LITERAL_STRING, code + start, index - start, line, col - (index - start));
      if (code[index] == '"') {
        mode = Mode_Normal;
        addToken(tokens, arena, TEMPLATE_END, "", 0, line, col - (index - start));
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
      addToken(tokens, arena, TEMPLATE_START, "", 0, line, col);
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

      addToken(tokens, arena, LITERAL_STRING, code + start, index - start, line, col - (index - start));
      NEXT();
      continue;
    }

    if (code[index] == '_' || (code[index] >= 'a' && code[index] <= 'z') || (code[index] >= 'A' && code[index] <= 'Z')) {
      int start = index;
      while (code[index] == '_' || (code[index] >= 'a' && code[index] <= 'z') || (code[index] >= 'A' && code[index] <= 'Z') ||
          (code[index] >= '0' && code[index] <= '9')) NEXT();

#define X(name, str) \
      if (memcmp(code + start, str, strlen(str)) == 0) { \
        addToken(tokens, arena, name, code + start, index - start, line, col - (index - start)); \
        for (int i = 0; i < strlen(str); i++) NEXT(); \
        continue; \
      }
      KEYWORDS
#undef X

      addToken(tokens, arena, IDENTIFIER, code + start, index - start, line, col - (index - start));
      continue;
    }

    if (code[index] >= '0' && code[index] <= '9') {
      int start = index;
      enum TokenType type = LITERAL_INTEGER;
      while (code[index] >= '0' && code[index] <= '9') NEXT();

      if (code[index] == '.') {
        NEXT();
        type = LITERAL_FLOAT;
        while (code[index] >= '0' && code[index] <= '9') NEXT();
      }

      addToken(tokens, arena, type, code + start, index - start, line, col - (index - start));
      continue;
    }

#define X(name, str) \
    if (strncmp(code + index, str, strlen(str)) == 0) { \
      addToken(tokens, arena, name, code + index, strlen(str), line, col); \
      for (int i = 0; i < strlen(str); i++) NEXT(); \
      continue; \
    }
    ASSIGNMENTS
    OPERATORS
    SYMBOLS
#undef X

    if (isspace(code[index])) {
      NEXT();
      continue;
    }

    errorLang(filename, line, col, "invalid character '%c'", code[index]);
  }

  addToken(tokens, arena, TOKEN_EOF, "", 0, line, col);
}

void showTokens(struct Tokens *tokens) {
  printf("=============================== { TOKENS } ===============================\n");
  for (int i = 0; i < tokens->length; i++) {
    printf(" %d:\n", i);
    printf("  type: %d\n", tokens->token[i].type);
    printf("  value: %.*s\n", tokens->token[i].string.length, tokens->token[i].string.start);
    printf("  line: %d\n", tokens->token[i].line);
    printf("  column: %d\n", tokens->token[i].column);
  }
  printf("=============================== { TOKENS } ===============================\n");
}
