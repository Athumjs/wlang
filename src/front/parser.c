#include "utils/tokens.h"
#include <front/parser.h>
#include <utils/error.h>
#include <string.h>

enum TokenType peek(int i, struct Tokens *tokens) {
  return tokens->token[i].type;
}

#define PEEK() peek(*i, tokens)

union Literal consume(int *i, struct Tokens *tokens, struct Program *program, enum TokenType type) {
  if (PEEK() != type) {
    errorLang(program->args->input_file, tokens->token[*i - 1].line, tokens->token[*i - 1].column, "expected '%s'", tk_names[type]);
  }

  (*i)++;
  return tokens->token[*i - 1].literal;
}

#define CONSUME(t) consume(i, tokens, program, t)

struct Expr *newExpr(int *i, struct Tokens *tokens, struct Program *program, enum ExprKind kind) {
  struct Expr *expr = arena_alloc(program->arena, sizeof(struct Expr));
  memset(expr, 0, sizeof(struct Expr));
  expr->kind = kind;
  expr->line = tokens->token[*i].line;
  expr->column = tokens->token[*i].column;
  return expr;
}

struct Expr *parseExpr(int *i, struct Tokens *tokens, struct Program *program);

struct Expr *parsePrimary(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_LPAREN) {
    CONSUME(TOKEN_LPAREN);
    struct Expr *expr = parseExpr(i, tokens, program);
    CONSUME(TOKEN_RPAREN);
    return expr;
  }

  else if (PEEK() == TOKEN_LBRACE) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Struct);
    CONSUME(TOKEN_LBRACE);

    if (PEEK() != TOKEN_RBRACE) {
      expr->expr_struct.properties_cap = 2;
      expr->expr_struct.properties = arena_alloc(program->arena, expr->expr_struct.properties_cap * sizeof(struct Property));

      while (true) {
        if (expr->expr_struct.properties_len == expr->expr_struct.properties_cap) {
          size_t oldCap = expr->expr_struct.properties_cap;
          expr->expr_struct.properties_cap *= 2;
          struct Property *temp = arena_alloc(program->arena, expr->expr_struct.properties_cap * sizeof(struct Property));
          memcpy(temp, expr->expr_struct.properties, oldCap * sizeof(struct Property));
          expr->expr_struct.properties = temp;
        }

        struct Property property = {
          .line = tokens->token[*i].line,
          .column = tokens->token[*i].column,
          .name = CONSUME(IDENTIFIER).string,
          .expr = parseExpr(i, tokens, program)
        };
        expr->expr_struct.properties[expr->expr_struct.properties_len++] = property;
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RBRACE);
    return expr;
  }

  else if (PEEK() == TOKEN_LBRACKET) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Array);
    CONSUME(TOKEN_LBRACKET);

    if (PEEK() != TOKEN_RBRACKET) {
      expr->expr_array.exprs_cap = 2;
      expr->expr_array.exprs = arena_alloc(program->arena, expr->expr_array.exprs_cap * sizeof(struct Expr *));

      while (true) {
        if (expr->expr_array.exprs_len == expr->expr_array.exprs_cap) {
          size_t oldCap = expr->expr_array.exprs_cap;
          expr->expr_array.exprs_cap *= 2;
          struct Expr **temp = arena_alloc(program->arena, expr->expr_array.exprs_cap * sizeof(struct Expr *));
          memcpy(temp, expr->expr_array.exprs, oldCap * sizeof(struct Property));
          expr->expr_array.exprs = temp;
        }

        expr->expr_array.exprs[expr->expr_array.exprs_len++] = parseExpr(i, tokens, program);
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RBRACKET);
    return expr;
  }

  else if (PEEK() == LITERAL_INTEGER || PEEK() == LITERAL_UINTEGER || PEEK() == LITERAL_FLOAT || PEEK() == LITERAL_DOUBLE ||
      PEEK() == LITERAL_CHAR || PEEK() == LITERAL_STRING || PEEK() == LITERAL_BOOLEAN) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Literal);
    expr->expr_literal.kind = PEEK();
    expr->expr_literal.literal = CONSUME(PEEK());
    return expr;
  }

  else if (PEEK() == IDENTIFIER) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Identifier);
    expr->expr_identifier = CONSUME(IDENTIFIER).string;
    return expr;
  }

  errorLang(program->args->input_file, tokens->token[*i - 1].line, tokens->token[*i - 1].column, "expected expression");
}

struct Expr *parseExpr(int *i, struct Tokens *tokens, struct Program *program) {
  return parseExpr(i, tokens, program);
}

struct Node *parseNode(int *i, struct Tokens *tokens, struct Program *program) {
  return parseNode(i, tokens, program);
}

void parser(struct Tokens *tokens, struct Program *program) {
  int index = 0;

  while (peek(index, tokens) != TOKEN_EOF) {
    if (program->length == program->capacity) {
      size_t oldCap = program->capacity;
      program->capacity *= 2;
      struct Node **temp = arena_alloc(program->arena, program->capacity * sizeof(struct Node *));
      memcpy(temp, program->nodes, oldCap * sizeof(struct Node *));
      program->nodes = temp;
    }

    program->nodes[program->length++] = parseNode(&index, tokens, program);
  }
}
