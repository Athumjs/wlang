#include <front/parser.h>
#include "parser.h"
#include <utils/error.h>
#include <string.h>

enum TokenType peek(int i, struct Tokens *tokens) {
  return tokens->token[i].type;
}

union Literal consume(int *i, struct Tokens *tokens, struct Program *program, enum TokenType type) {
  if (PEEK() != type) {
    errorLang(program->filename, tokens->token[*i - 1].line, tokens->token[*i - 1].column, "expected '%s'", tk_names[type]);
  }

  (*i)++;
  return tokens->token[*i - 1].literal;
}

void parser(struct Tokens *tokens, struct Program *program) {
  int index = 0;

  while (peek(index, tokens) != TOKEN_EOF) {
    if (program->length == program->capacity) {
      size_t oldCap = program->capacity;
      program->capacity *= 2;
      struct Decl **temp = arena_alloc(program->arena, program->capacity * sizeof(struct Decl *));
      memcpy(temp, program->decls, oldCap * sizeof(struct Decl *));
      program->decls = temp;
    }

    struct Decl *decl = parseDecl(&index, tokens, program);

    if (decl == NULL) {
      errorLang(program->filename, tokens->token[index].line, tokens->token[index].column, "'%s' is not a declaration", tk_names[peek(index, tokens)]);
    }

    program->decls[program->length++] = decl;
  }
}
