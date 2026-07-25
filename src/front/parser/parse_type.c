#include "parser.h"
#include <string.h>

struct Type *parseType(int *i, struct Tokens *tokens, struct Program *program) {
  struct Type *type = arena_alloc(program->arena, sizeof(struct Type));

  if (PEEK() == IDENTIFIER) {
    type->kind = Type_Named;
    type->type_named.name = CONSUME(IDENTIFIER).string;
    
    while (1) {
      if (PEEK() == TOKEN_LBRACKET) {
        struct Type *temp = arena_alloc(program->arena, sizeof(struct Type));
        temp->kind = Type_Array;
        temp->type_array.base = type;
        CONSUME(TOKEN_LBRACKET);
        temp->type_array.expr = parseExpr(i, tokens, program);
        CONSUME(TOKEN_RBRACKET);
        type = temp;
      }

      else if (PEEK() == TOKEN_ASTERISK) {
        struct Type *temp = arena_alloc(program->arena, sizeof(struct Type));
        temp->kind = Type_Pointer;
        temp->type_pointer.base = type;
        CONSUME(TOKEN_ASTERISK);
        type = temp;
      }

      else break;
    }
  }

  else if (PEEK() == TOKEN_LPAREN) {
    type->kind = Type_Function;
    type->type_function.params_cap = 0;
    type->type_function.params = NULL;
    type->type_function.params_len = 0;
    CONSUME(TOKEN_LPAREN);

    if (PEEK() != TOKEN_RPAREN) {
      type->type_function.params_cap = 1;
      type->type_function.params = arena_alloc(program->arena, type->type_function.params_cap * sizeof(struct Type *));

      while (1) {
        if (type->type_function.params_len == type->type_function.params_cap) {
          size_t oldCap = type->type_function.params_cap;
          type->type_function.params_cap *= 2;
          struct Type **temp = arena_alloc(program->arena, type->type_function.params_cap * sizeof(struct Type *));
          memcpy(temp, type->type_function.params, oldCap * sizeof(struct Type *));
          type->type_function.params = temp;
        }

        type->type_function.params[type->type_function.params_len++] = parseType(i, tokens, program);
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RPAREN);
    CONSUME(TOKEN_ARROW);
    type->type_function.retType = parseType(i, tokens, program);
  }

  return type;
}
