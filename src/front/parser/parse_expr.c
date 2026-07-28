#include "parser.h"
#include <string.h>
#include <utils/error.h>

struct Type *exprType(int *i, struct Tokens *tokens, struct Program *program) {
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

        type->type_function.params[type->type_function.params_len++] = exprType(i, tokens, program);
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RPAREN);
    CONSUME(TOKEN_ARROW);
    type->type_function.retType = exprType(i, tokens, program);
  }

  return type;
}

struct Expr *exprPrimary(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_LPAREN) {
    CONSUME(TOKEN_LPAREN);
    struct Expr *expr = parseExpr(i, tokens, program);
    CONSUME(TOKEN_RPAREN);
    return expr;
  }

  else if (PEEK() == TOKEN_COLON) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Callback);
    CONSUME(TOKEN_COLON);
    CONSUME(TOKEN_LPAREN);

    if (PEEK() != TOKEN_RPAREN) {
      expr->expr_callback.params_cap = 2;
      expr->expr_callback.params = arena_alloc(program->arena, expr->expr_callback.params_cap * sizeof(struct Param));

      while (1) {
        if (expr->expr_callback.params_len == expr->expr_callback.params_cap) {
          size_t oldCap = expr->expr_callback.params_cap;
          expr->expr_callback.params_cap *= 2;
          struct Param *temp = arena_alloc(program->arena, expr->expr_callback.params_cap * sizeof(struct Param));
          memcpy(temp, expr->expr_callback.params, oldCap * sizeof(struct Param));
          expr->expr_callback.params = temp;
        }

        struct Param param = {
          .line = tokens->token[*i].line,
          .column = tokens->token[*i].column,
          .name = CONSUME(IDENTIFIER).string,
          .type = NULL
        };

        if (PEEK() == TOKEN_COLON) {
          CONSUME(TOKEN_COLON);
          param.type = parseType(i, tokens, program);
        }

        expr->expr_callback.params[expr->expr_callback.params_len++] = param;
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RPAREN);

    if (PEEK() == TOKEN_COLON) {
      CONSUME(TOKEN_COLON);
      expr->expr_callback.retType = parseType(i, tokens, program);
    }

    expr->expr_callback.body = parseStmt(i, tokens, program);
    return expr;
  }

  else if (PEEK() == TOKEN_LBRACE) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Struct);
    CONSUME(TOKEN_LBRACE);

    if (PEEK() != TOKEN_RBRACE) {
      expr->expr_struct.fields_cap = 2;
      expr->expr_struct.fields = arena_alloc(program->arena, expr->expr_struct.fields_cap * sizeof(struct Field));

      while (1) {
        if (expr->expr_struct.fields_len == expr->expr_struct.fields_cap) {
          size_t oldCap = expr->expr_struct.fields_cap;
          expr->expr_struct.fields_cap *= 2;
          struct Field *temp = arena_alloc(program->arena, expr->expr_struct.fields_cap * sizeof(struct Field));
          memcpy(temp, expr->expr_struct.fields, oldCap * sizeof(struct Field));
          expr->expr_struct.fields = temp;
        }

        struct Field field = {
          .line = tokens->token[*i].line,
          .column = tokens->token[*i].column,
          .name = CONSUME(IDENTIFIER).string,
          .expr = NULL
        };
        CONSUME(TOKEN_COLON);
        field.expr = parseExpr(i, tokens, program);
        expr->expr_struct.fields[expr->expr_struct.fields_len++] = field;
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

      while (1) {
        if (expr->expr_array.exprs_len == expr->expr_array.exprs_cap) {
          size_t oldCap = expr->expr_array.exprs_cap;
          expr->expr_array.exprs_cap *= 2;
          struct Expr **temp = arena_alloc(program->arena, expr->expr_array.exprs_cap * sizeof(struct Expr *));
          memcpy(temp, expr->expr_array.exprs, oldCap * sizeof(struct Expr *));
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

  else if (PEEK() == TOKEN_THIS) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_This);
    CONSUME(TOKEN_THIS);
    CONSUME(TOKEN_DOT);
    expr->expr_this = CONSUME(IDENTIFIER).string;
    return expr;
  }

  else if (PEEK() == IDENTIFIER) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Identifier);
    expr->expr_identifier = CONSUME(IDENTIFIER).string;
    return expr;
  }

  errorLang(program->filename, tokens->token[*i].line, tokens->token[*i].column, "expected expression");
}

void exprCall(int *i, struct Tokens *tokens, struct Program *program, struct Expr **expr) {
  struct Expr *temp = newExpr(i, tokens, program, Expr_Call);
  temp->expr_call.callee = *expr;
  CONSUME(TOKEN_LPAREN);

  if (PEEK() != TOKEN_RPAREN) {
    temp->expr_call.args_cap = 2;
    temp->expr_call.args = arena_alloc(program->arena, temp->expr_call.args_cap * sizeof(struct Expr *));

    while (1) {
      if (temp->expr_call.args_len == temp->expr_call.args_cap) {
        size_t oldCap = temp->expr_call.args_cap;
        temp->expr_call.args_cap *= 2;
        struct Expr **t = arena_alloc(program->arena, temp->expr_call.args_cap * sizeof(struct Expr *));
        memcpy(t, temp->expr_call.args, oldCap * sizeof(struct Expr *));
        temp->expr_call.args = t;
      }

      temp->expr_call.args[temp->expr_call.args_len++] = parseExpr(i, tokens, program);
      if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
      else break;
    }
  }

  CONSUME(TOKEN_RPAREN);
  *expr = temp;
}

void exprIndex(int *i, struct Tokens *tokens, struct Program *program, struct Expr **expr) {
  struct Expr *temp = newExpr(i, tokens, program, Expr_Index);
  temp->expr_index.base = *expr;
  CONSUME(TOKEN_LBRACKET);
  temp->expr_index.index = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RBRACKET);
  *expr = temp;
}

void exprMember(int *i, struct Tokens *tokens, struct Program *program, struct Expr **expr) {
  struct Expr *temp = newExpr(i, tokens, program, Expr_Member);
  CONSUME(TOKEN_DOT);
  temp->expr_member.obj = *expr;
  temp->expr_member.member = exprPrimary(i, tokens, program);
  *expr = temp;
}

struct Expr *exprPostfix(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprPrimary(i, tokens, program);

  while (1) {
    if (PEEK() == TOKEN_LPAREN) exprCall(i, tokens, program, &expr);
    else if (PEEK() == TOKEN_LBRACKET) exprIndex(i, tokens, program, &expr);
    else if (PEEK() == TOKEN_DOT) exprMember(i, tokens, program, &expr);
    else break;
  }

  return expr;
}

struct Expr *exprUnary(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_NOT || PEEK() == TOKEN_BIT_NOT || PEEK() == TOKEN_MINUS ||
      PEEK() == TOKEN_INCREMENT || PEEK() == TOKEN_DECREMENT || PEEK() == TOKEN_ASTERISK || PEEK() == TOKEN_BIT_AND) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Unary);
    expr->expr_unary.op = PEEK();
    expr->expr_unary.prefix = 1;
    CONSUME(PEEK());
    expr->expr_unary.arg = exprPrimary(i, tokens, program);
    return expr;
  }

  struct Expr *expr = exprPostfix(i, tokens, program);

  if (PEEK() == TOKEN_INCREMENT || PEEK() == TOKEN_DECREMENT) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Unary);
    temp->expr_unary.arg = expr;
    temp->expr_unary.op = PEEK();
    temp->expr_unary.prefix = 0;
    CONSUME(PEEK());
    expr = temp;
  }
  
  return expr;
}

struct Expr *exprCast(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprUnary(i, tokens, program);

  if (PEEK() == TOKEN_AS) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Cast);
    temp->expr_cast.value = expr;
    CONSUME(TOKEN_AS);
    temp->expr_cast.type = exprType(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBinaryGreater(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprCast(i, tokens, program);

  while (PEEK() == TOKEN_ASTERISK || PEEK() == TOKEN_SLASH || PEEK() == TOKEN_MOD) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprCast(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBinaryLess(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBinaryGreater(i, tokens, program);

  while (PEEK() == TOKEN_PLUS || PEEK() == TOKEN_MINUS) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprBinaryGreater(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBitwiseShift(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBinaryLess(i, tokens, program);

  while (PEEK() == TOKEN_SHIFT_LEFT || PEEK() == TOKEN_SHIFT_RIGHT) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprBinaryLess(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBitwiseAND(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBitwiseShift(i, tokens, program);

  while (PEEK() == TOKEN_BIT_AND) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_AND;
    CONSUME(TOKEN_BIT_AND);
    temp->expr_binary.right = exprBitwiseShift(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBitwiseXOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBitwiseAND(i, tokens, program);

  while (PEEK() == TOKEN_BIT_XOR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_XOR;
    CONSUME(TOKEN_BIT_XOR);
    temp->expr_binary.right = exprBitwiseAND(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprBitwiseOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBitwiseXOR(i, tokens, program);

  while (PEEK() == TOKEN_BIT_OR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_OR;
    CONSUME(TOKEN_BIT_OR);
    temp->expr_binary.right = exprBitwiseXOR(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprCompareInequality(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprBitwiseOR(i, tokens, program);

  while (PEEK() == TOKEN_GT || PEEK() == TOKEN_GE || PEEK() == TOKEN_LT || PEEK() == TOKEN_LE) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Compare);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprBitwiseOR(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprCompareEquality(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprCompareInequality(i, tokens, program);

  while (PEEK() == TOKEN_EQ || PEEK() == TOKEN_NE) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Compare);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprCompareInequality(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprLogicalAND(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprCompareEquality(i, tokens, program);

  while (PEEK() == TOKEN_AND) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Logical);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_AND;
    CONSUME(TOKEN_AND);
    temp->expr_binary.right = exprCompareEquality(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprLogicalOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprLogicalAND(i, tokens, program);

  while (PEEK() == TOKEN_OR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Logical);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_OR;
    CONSUME(TOKEN_OR);
    temp->expr_binary.right = exprLogicalAND(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *exprAssign(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = exprLogicalOR(i, tokens, program);

  if (PEEK() == TOKEN_ASSIGN
#define X(name, str) || PEEK() == name
      ASSIGNMENTS
#undef X
      ) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Assign);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = exprAssign(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseExpr(int *i, struct Tokens *tokens, struct Program *program) {
  return exprAssign(i, tokens, program);
}
