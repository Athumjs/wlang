#include <front/parser.h>
#include <stdarg.h>
#include <stdio.h>
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

struct Type *parseType(int *i, struct Tokens *tokens, struct Program *program) {
  struct Type *type = arena_alloc(program->arena, sizeof(struct Type));

  if (PEEK() == IDENTIFIER) {
    type->kind = Type_User;
    type->type_user.name = CONSUME(IDENTIFIER).string;

    if (PEEK() == TOKEN_LBRACKET) {
      struct Type *temp = arena_alloc(program->arena, sizeof(struct Type));
      temp->kind = Type_Array;
      temp->type_array.base = type;
      CONSUME(TOKEN_LBRACKET);
      temp->type_array.expr = parseExpr(i, tokens, program);
      CONSUME(TOKEN_RBRACKET);
      type = temp;
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

      while (1) {
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
          .expr = NULL
        };
        CONSUME(TOKEN_COLON);
        property.expr = parseExpr(i, tokens, program);
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

  else if (PEEK() == IDENTIFIER) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Identifier);
    expr->expr_identifier = CONSUME(IDENTIFIER).string;
    return expr;
  }

  errorLang(program->args->input_file, tokens->token[*i].line, tokens->token[*i].column, "expected expression");
}

struct Expr *parseCall(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parsePrimary(i, tokens, program);

  while (PEEK() == TOKEN_LPAREN) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Call);
    temp->expr_call.callee = expr;
    CONSUME(TOKEN_LPAREN);

    if (PEEK() != TOKEN_RPAREN) {
      temp->expr_call.args_cap = 2;
      temp->expr_call.args = arena_alloc(program->arena, temp->expr_call.args_cap * sizeof(struct Expr *));

      while (1) {
        if (expr->expr_call.args_len == expr->expr_call.args_cap) {
          size_t oldCap = expr->expr_call.args_cap;
          expr->expr_call.args_cap *= 2;
          struct Expr **temp = arena_alloc(program->arena, expr->expr_call.args_cap * sizeof(struct Expr *));
          memcpy(temp, expr->expr_call.args, oldCap * sizeof(struct Expr *));
          expr->expr_call.args = temp;
        }

        expr->expr_call.args[expr->expr_call.args_len++] = parseExpr(i, tokens, program);
        if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
        else break;
      }
    }

    CONSUME(TOKEN_RPAREN);
    expr = temp;
  }

  return expr;
}

struct Expr *parseMember(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseCall(i, tokens, program);

  while (PEEK() == TOKEN_DOT) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Member);
    CONSUME(TOKEN_DOT);
    temp->expr_member.obj = expr;
    temp->expr_member.member = parseCall(i, tokens, program);
    expr = temp;
  }
  
  return expr;
}

struct Expr *parseUnary(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_NOT || PEEK() == TOKEN_BIT_NOT || PEEK() == TOKEN_MINUS || PEEK() == TOKEN_INCREMENT || PEEK() == TOKEN_DECREMENT) {
    struct Expr *expr = newExpr(i, tokens, program, Expr_Unary);
    expr->expr_unary.op = PEEK();
    expr->expr_unary.prefix = 1;
    CONSUME(PEEK());
    expr->expr_unary.arg = parseMember(i, tokens, program);
    return expr;
  }

  struct Expr *expr = parseMember(i, tokens, program);

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

struct Expr *parseCast(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseUnary(i, tokens, program);

  if (PEEK() == TOKEN_AS) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Cast);
    temp->expr_cast.value = expr;
    CONSUME(TOKEN_AS);
    temp->expr_cast.type = parseType(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBinaryGreater(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseCast(i, tokens, program);

  while (PEEK() == TOKEN_ASTERISK || PEEK() == TOKEN_SLASH || PEEK() == TOKEN_MOD) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseCast(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBinaryLess(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBinaryGreater(i, tokens, program);

  while (PEEK() == TOKEN_PLUS || PEEK() == TOKEN_MINUS) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseBinaryGreater(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBitwiseShift(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBinaryLess(i, tokens, program);

  while (PEEK() == TOKEN_SHIFT_LEFT || PEEK() == TOKEN_SHIFT_RIGHT) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseBinaryLess(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBitwiseAND(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBitwiseShift(i, tokens, program);

  while (PEEK() == TOKEN_BIT_AND) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_AND;
    CONSUME(TOKEN_BIT_AND);
    temp->expr_binary.right = parseBitwiseShift(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBitwiseXOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBitwiseAND(i, tokens, program);

  while (PEEK() == TOKEN_BIT_XOR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_XOR;
    CONSUME(TOKEN_BIT_XOR);
    temp->expr_binary.right = parseBitwiseAND(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseBitwiseOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBitwiseXOR(i, tokens, program);

  while (PEEK() == TOKEN_BIT_OR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Binary);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_BIT_OR;
    CONSUME(TOKEN_BIT_OR);
    temp->expr_binary.right = parseBitwiseXOR(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseCompareInequality(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseBitwiseOR(i, tokens, program);

  while (PEEK() == TOKEN_GT || PEEK() == TOKEN_GE || PEEK() == TOKEN_LT || PEEK() == TOKEN_LE) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Compare);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseBitwiseOR(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseCompareEquality(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseCompareInequality(i, tokens, program);

  while (PEEK() == TOKEN_EQ || PEEK() == TOKEN_NE) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Compare);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseCompareInequality(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseLogicalAND(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseCompareEquality(i, tokens, program);

  while (PEEK() == TOKEN_AND) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Logical);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_AND;
    CONSUME(TOKEN_AND);
    temp->expr_binary.right = parseCompareEquality(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseLogicalOR(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseLogicalAND(i, tokens, program);

  while (PEEK() == TOKEN_OR) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Logical);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = TOKEN_OR;
    CONSUME(TOKEN_OR);
    temp->expr_binary.right = parseLogicalAND(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseAssign(int *i, struct Tokens *tokens, struct Program *program) {
  struct Expr *expr = parseLogicalOR(i, tokens, program);

  if (PEEK() == TOKEN_ASSIGN
#define X(name, str) || PEEK() == name
      ASSIGNMENTS
#undef X
      ) {
    struct Expr *temp = newExpr(i, tokens, program, Expr_Assign);
    temp->expr_binary.left = expr;
    temp->expr_binary.op = PEEK();
    CONSUME(PEEK());
    temp->expr_binary.right = parseAssign(i, tokens, program);
    expr = temp;
  }

  return expr;
}

struct Expr *parseExpr(int *i, struct Tokens *tokens, struct Program *program) {
  return parseAssign(i, tokens, program);
}

struct Node *newNode(int *i, struct Tokens *tokens, struct Program *program, enum NodeKind kind) {
  struct Node *node = arena_alloc(program->arena, sizeof(struct Node));
  memset(node, 0, sizeof(struct Node));
  node->kind = kind;
  node->line = tokens->token[*i].line;
  node->column = tokens->token[*i].column;
  return node;
}

struct Node *parseNode(int *i, struct Tokens *tokens, struct Program *program);

struct Node *parseImport(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Import);
  CONSUME(TOKEN_IMPORT);
  node->node_import = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  return node;
}

struct Node *parsePublic(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Public);
  CONSUME(TOKEN_PUBLIC);
  node->node_public = parseNode(i, tokens, program);
  return node;
}

struct Node *parseVariable(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Variable);
  node->node_variable.vars_cap = 2;
  node->node_variable.vars = arena_alloc(program->arena, node->node_variable.vars_cap * sizeof(struct Var));

  if (PEEK() == TOKEN_CONST) {
    CONSUME(TOKEN_CONST);
    node->node_variable.isConst = 1;
  } else CONSUME(TOKEN_VAR);

  while (1) {
    if (node->node_variable.vars_len == node->node_variable.vars_cap) {
      size_t oldCap = node->node_variable.vars_cap;
      node->node_variable.vars_cap *= 2;
      struct Var *temp = arena_alloc(program->arena, node->node_variable.vars_cap * sizeof(struct Var));
      memcpy(temp, node->node_variable.vars, oldCap * sizeof(struct Var));
      node->node_variable.vars = temp;
    }

    struct Var var = {
      .line = tokens->token[*i].line,
      .column = tokens->token[*i].column,
      .name = CONSUME(IDENTIFIER).string,
      .type = NULL,
      .expr = NULL
    };

    if (PEEK() == TOKEN_COLON) {
      CONSUME(TOKEN_COLON);
      var.type = parseType(i, tokens, program);
    }

    if (PEEK() == TOKEN_ASSIGN) {
      CONSUME(TOKEN_ASSIGN);
      var.expr = parseExpr(i, tokens, program);
    }

    node->node_variable.vars[node->node_variable.vars_len++] = var;
    if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
    else break;
  }

  CONSUME(TOKEN_SEMICOLON);
  return node;
}

struct Node *parseBlock(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Block);
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    node->node_block.nodes_cap = 64;
    node->node_block.nodes = arena_alloc(program->arena, node->node_block.nodes_cap * sizeof(struct Node *));

    while (PEEK() != TOKEN_RBRACE) {
      if (node->node_block.nodes_len == node->node_block.nodes_cap) {
        size_t oldCap = node->node_block.nodes_cap;
        node->node_block.nodes_cap *= 2;
        struct Node **temp = arena_alloc(program->arena, node->node_block.nodes_cap * sizeof(struct Node *));
        memcpy(temp, node->node_block.nodes, oldCap * sizeof(struct Node *));
        node->node_block.nodes = temp;
      }

      node->node_block.nodes[node->node_block.nodes_len++] = parseNode(i, tokens, program);
    }
  }

  CONSUME(TOKEN_RBRACE);
  return node;
}

struct Node *parseFunction(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Function);
  CONSUME(TOKEN_SET);
  node->node_function.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LPAREN);

  if (PEEK() != TOKEN_RPAREN) {
    node->node_function.params_cap = 2;
    node->node_function.params = arena_alloc(program->arena, node->node_function.params_cap * sizeof(struct Label));

    while (1) {
      if (node->node_function.params_len == node->node_function.params_cap) {
        size_t oldCap = node->node_function.params_cap;
        node->node_function.params_cap *= 2;
        struct Label *temp = arena_alloc(program->arena, node->node_function.params_cap * sizeof(struct Label));
        memcpy(temp, node->node_function.params, oldCap * sizeof(struct Label));
        node->node_function.params = temp;
      }

      struct Label param = {
        .line = tokens->token[*i].line,
        .column = tokens->token[*i].column,
        .name = CONSUME(IDENTIFIER).string,
        .type = NULL
      };

      if (PEEK() == TOKEN_COLON) {
        CONSUME(TOKEN_COLON);
        param.type = parseType(i, tokens, program);
      }

      node->node_function.params[node->node_function.params_len++] = param;
      if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
      else break;
    }
  }

  CONSUME(TOKEN_RPAREN);
  
  if (PEEK() == TOKEN_COLON) {
    CONSUME(TOKEN_COLON);
    node->node_function.retType = parseType(i, tokens, program);
  }

  node->node_function.body = parseBlock(i, tokens, program);
  return node;
}

struct Node *parseCondition(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Condition);
  CONSUME(TOKEN_IF);
  CONSUME(TOKEN_LPAREN);
  node->node_condition.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  node->node_condition.trueBody = parseNode(i, tokens, program);

  if (PEEK() == TOKEN_ELSE) {
    CONSUME(TOKEN_ELSE);
    node->node_condition.falseBody = parseNode(i, tokens, program);
  }

  return node;
}

struct Node *parseLoopWhile(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_LoopWhile);

  if (PEEK() == TOKEN_DO) {
    node->node_loopWhile.doWhile = 1;
    CONSUME(TOKEN_DO);
    node->node_loopWhile.body = parseBlock(i, tokens, program);
    CONSUME(TOKEN_WHILE);
    CONSUME(TOKEN_LPAREN);
    node->node_loopWhile.condition = parseExpr(i, tokens, program);
    CONSUME(TOKEN_RPAREN);
    CONSUME(TOKEN_SEMICOLON);
    return node;
  }

  CONSUME(TOKEN_WHILE);
  CONSUME(TOKEN_LPAREN);
  node->node_loopWhile.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  node->node_loopWhile.body = parseNode(i, tokens, program);
  return node;
}

struct Node *parseLoopFor(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_LoopFor);
  CONSUME(TOKEN_FOR);
  CONSUME(TOKEN_LPAREN);
  node->node_loopFor.init = parseNode(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  node->node_loopFor.condition = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  node->node_loopFor.uptade = parseExpr(i, tokens, program);
  CONSUME(TOKEN_RPAREN);
  node->node_loopFor.body = parseNode(i, tokens, program);
  return node;
}

struct Node *parseReturn(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Return);
  CONSUME(TOKEN_RETURN);
  if (PEEK() != TOKEN_SEMICOLON) node->node_return = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  return node;
}

struct Node *parseNodeEmpty(int *i, struct Tokens *tokens, struct Program *program, enum NodeKind kind) {
  struct Node *node = newNode(i, tokens, program, kind);
  CONSUME(PEEK());
  return node;
}

struct Node *parseEnum(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Enum);
  CONSUME(TOKEN_ENUM);
  node->node_enum.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    node->node_enum.elements_cap = 2;
    node->node_enum.elements = arena_alloc(program->arena, node->node_enum.elements_cap * sizeof(struct Element));

    while (1) {
      if (node->node_enum.elements_len == node->node_enum.elements_cap) {
        size_t oldCap = node->node_enum.elements_cap;
        node->node_enum.elements_cap *= 2;
        struct Element *temp = arena_alloc(program->arena, node->node_enum.elements_cap * sizeof(struct Element));
        memcpy(temp, node->node_enum.elements, oldCap * sizeof(struct Element));
        node->node_enum.elements = temp;
      }

      struct Element element = {
        .line = tokens->token[*i].line,
        .column = tokens->token[*i].column,
        .name = CONSUME(IDENTIFIER).string,
        .expr = NULL
      };

      if (PEEK() == TOKEN_ASSIGN) {
        CONSUME(TOKEN_ASSIGN);
        element.expr = parseExpr(i, tokens, program);
      }

      node->node_enum.elements[node->node_enum.elements_len++] = element;

      if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
      else break;
    }
  }

  CONSUME(TOKEN_RBRACE);
  if (PEEK() == TOKEN_SEMICOLON) CONSUME(TOKEN_SEMICOLON);
  return node;
}

void resolveStructMethods(int *i, struct Tokens *tokens, struct Program *program, struct Node *node) {
	if (node->node_struct.methods_len == node->node_struct.properties_cap) {
	  size_t oldCap = node->node_struct.methods_cap;
	  node->node_struct.methods_cap *= 2;
	  struct Method *temp = arena_alloc(program->arena, node->node_struct.methods_cap * sizeof(struct Property));
	  memcpy(temp, node->node_struct.methods, oldCap * sizeof(struct Method));
	  node->node_struct.methods = temp;
	}

  CONSUME(TOKEN_SET);
	
	struct Method method = {
	  .line = tokens->token[*i].line,
	  .column = tokens->token[*i].column,
	  .name = CONSUME(IDENTIFIER).string,
	  .params_cap = 0,
	  .params = NULL,
	  .params_len = 0
	};
	CONSUME(TOKEN_LPAREN);
	
	if (PEEK() != TOKEN_RPAREN) {
	  method.params_cap = 2;
	  method.params = arena_alloc(program->arena, method.params_cap * sizeof(struct Label));
	
	  while (1) {
	    if (method.params_len == method.params_cap) {
	      size_t oldCap = method.params_cap;
	      method.params_cap *= 2;
	      struct Label *temp = arena_alloc(program->arena, method.params_cap * sizeof(struct Label));
	      memcpy(temp, method.params, oldCap * sizeof(struct Label));
	      method.params = temp;
	    }
	
	    struct Label param = {
	      .line = tokens->token[*i].line,
	      .column = tokens->token[*i].column,
	      .name = CONSUME(IDENTIFIER).string,
	      .type = NULL
	    };
	
	    if (PEEK() == TOKEN_COLON) {
	      CONSUME(TOKEN_COLON);
	      param.type = parseType(i, tokens, program);
	    }
	
	    method.params[method.params_len++] = param;
	    if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
	    else break;
	  }
	}
	
	CONSUME(TOKEN_RPAREN);

  if (PEEK() == TOKEN_COLON) {
    CONSUME(TOKEN_COLON);
    method.retType = parseType(i, tokens, program);
  }

	method.body = parseBlock(i, tokens, program);
	node->node_struct.methods[node->node_struct.methods_len++] = method;
  if (PEEK() != TOKEN_RBRACE) resolveStructMethods(i, tokens, program, node);
}

void resolveStructProperties(int *i, struct Tokens *tokens, struct Program *program, struct Node *node) {
  if (PEEK() == TOKEN_SET) {
    node->node_struct.methods_cap = 2;
    node->node_struct.methods = arena_alloc(program->arena, node->node_struct.methods_cap * sizeof(struct Method));
    return resolveStructMethods(i, tokens, program, node);
  }

	if (node->node_struct.properties_len == node->node_struct.properties_cap) {
	  size_t oldCap = node->node_struct.properties_cap;
	  node->node_struct.properties_cap *= 2;
	  struct Label *temp = arena_alloc(program->arena, node->node_struct.properties_cap * sizeof(struct Label));
	  memcpy(temp, node->node_struct.properties, oldCap * sizeof(struct Label));
	  node->node_struct.properties = temp;
  }
	
	struct Label property = {
	  .line = tokens->token[*i].line,
	  .column = tokens->token[*i].column,
	  .name = CONSUME(IDENTIFIER).string,
    .type = NULL
	};

  CONSUME(TOKEN_COLON);
  property.type = parseType(i, tokens, program);
	
	node->node_struct.properties[node->node_struct.properties_len++] = property;
	if (PEEK() == TOKEN_SEMICOLON) {
    CONSUME(TOKEN_SEMICOLON);
    resolveStructProperties(i, tokens, program, node);
  }
}

struct Node *parseStruct(int *i, struct Tokens *tokens, struct Program *program) {
  struct Node *node = newNode(i, tokens, program, Node_Struct);
  CONSUME(TOKEN_STRUCT);
  node->node_struct.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    node->node_struct.properties_cap = 2;
    node->node_struct.properties = arena_alloc(program->arena, node->node_struct.properties_cap * sizeof(struct Property));
    resolveStructProperties(i, tokens, program, node);
  }

  CONSUME(TOKEN_RBRACE);
  if (PEEK() == TOKEN_SEMICOLON) CONSUME(TOKEN_SEMICOLON);
  return node;
}

struct Node *parseNode(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_IMPORT) return parseImport(i, tokens, program);
  else if (PEEK() == TOKEN_PUBLIC) return parsePublic(i, tokens, program);
  else if (PEEK() == TOKEN_CONST || PEEK() == TOKEN_VAR) return parseVariable(i, tokens, program);
  else if (PEEK() == TOKEN_SET) return parseFunction(i, tokens, program);
  else if (PEEK() == TOKEN_IF) return parseCondition(i, tokens, program);
  else if (PEEK() == TOKEN_DO || PEEK() == TOKEN_WHILE) return parseLoopWhile(i, tokens, program);
  else if (PEEK() == TOKEN_FOR) return parseLoopFor(i, tokens, program);
  else if (PEEK() == TOKEN_RETURN) return parseReturn(i, tokens, program);
  else if (PEEK() == TOKEN_CONTINUE) return parseNodeEmpty(i, tokens, program, Node_Continue);
  else if (PEEK() == TOKEN_BREAK) return parseNodeEmpty(i, tokens, program, Node_Break);
  else if (PEEK() == TOKEN_ENUM) return parseEnum(i, tokens, program);
  else if (PEEK() == TOKEN_STRUCT) return parseStruct(i, tokens, program);
  else if (PEEK() == TOKEN_LBRACE) return parseBlock(i, tokens, program);
  else {
    struct Node *node = newNode(i, tokens, program, Node_Expr);
    node->node_expr = parseExpr(i, tokens, program);
    CONSUME(TOKEN_SEMICOLON);
    return node;
  }
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

void showExpr(struct Expr *expr, int tab);

void printTab(int tab, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  for (int i = 0; i < tab; i++) putchar(' ');
  printf("-> ");
  vprintf(msg, args);
  va_end(args);
}

void showType(struct Type *type, int tab) {
  if (type == NULL) return printTab(tab, "(null)\n");

  else if (type->kind == Type_User) {
    printTab(tab, "Type\n");
    printTab(tab, "value: %.*s\n", type->type_user.name.length, type->type_user.name.start);
  }

  else if (type->kind == Type_Array) {
    printTab(tab, "TypeArray\n");
    printTab(tab, "Base:\n");
    showType(type->type_array.base, tab + 2);
    printTab(tab, "Expression:\n");
    showExpr(type->type_array.expr, tab + 2);
  }

  else if (type->kind == Type_Function) {
    printTab(tab, "TypeFunction\n");
    for (int i = 0; i < type->type_function.params_len; i++) {
      printTab(tab, "Label %d:\n", i);
      showType(type->type_function.params[i], tab + 2);
    }
    printTab(tab, "Return Type:\n");
    showType(type->type_function.retType, tab + 2);
  }
}

void showExpr(struct Expr *expr, int tab) {
  if (expr == NULL) return printTab(tab, "(null)\n");

  else if (expr->kind == Expr_Assign) {
    printTab(tab, "AssignmentExpression\n");
    printTab(tab, "Left:\n");
    showExpr(expr->expr_binary.left, tab + 2);
    printTab(tab, "Operator: %s\n", tk_names[expr->expr_binary.op]);
    printTab(tab, "Right:\n");
    showExpr(expr->expr_binary.right, tab + 2);
  }

  else if (expr->kind == Expr_Logical) {
    printTab(tab, "LogicalExpression\n");
    printTab(tab, "Left:\n");
    showExpr(expr->expr_binary.left, tab + 2);
    printTab(tab, "Operator: %s\n", tk_names[expr->expr_binary.op]);
    printTab(tab, "Right:\n");
    showExpr(expr->expr_binary.right, tab + 2);
  }

  else if (expr->kind == Expr_Compare) {
    printTab(tab, "CompareExpression\n");
    printTab(tab, "Left:\n");
    showExpr(expr->expr_binary.left, tab + 2);
    printTab(tab, "Operator: %s\n", tk_names[expr->expr_binary.op]);
    printTab(tab, "Right:\n");
    showExpr(expr->expr_binary.right, tab + 2);
  }

  else if (expr->kind == Expr_Binary) {
    printTab(tab, "BinaryExpression\n");
    printTab(tab, "Left:\n");
    showExpr(expr->expr_binary.left, tab + 2);
    printTab(tab, "Operator: %s\n", tk_names[expr->expr_binary.op]);
    printTab(tab, "Right:\n");
    showExpr(expr->expr_binary.right, tab + 2);
  }

  else if (expr->kind == Expr_Cast) {
    printTab(tab, "CastExpression\n");
    printTab(tab, "Value:\n");
    showExpr(expr->expr_cast.value, tab + 2);
    printTab(tab, "Type:\n");
    showType(expr->expr_cast.type, tab + 2);
  }

  else if (expr->kind == Expr_Unary) {
    printTab(tab, "UnaryExpression\n");
    printTab(tab, "Argument:\n");
    showExpr(expr->expr_unary.arg, tab + 2);
    printTab(tab, "Operator: %s\n", tk_names[expr->expr_unary.op]);
    printTab(tab, "IsPrefix: %d\n", expr->expr_unary.prefix);
  }

  else if (expr->kind == Expr_Member) {
    printTab(tab, "MemberExpression\n");
    printTab(tab, "Object:\n");
    showExpr(expr->expr_member.obj, tab + 2);
    printTab(tab, "Member:\n");
    showExpr(expr->expr_member.member, tab + 2);
  }

  else if (expr->kind == Expr_Call) {
    printTab(tab, "CallExpression\n");
    printTab(tab, "Callee:\n");
    showExpr(expr->expr_call.callee, tab + 2);
    for (int i = 0; i < expr->expr_call.args_len; i++) {
      printTab(tab, "Argument %d:\n", i);
      showExpr(expr->expr_call.args[i], tab + 2);
    }
  }

  else if (expr->kind == Expr_Struct) {
    printTab(tab, "StructExpression\n");
    for (int i = 0; i < expr->expr_struct.properties_len; i++) {
      printTab(tab, "Property %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", expr->expr_struct.properties[i].name.length, expr->expr_struct.properties[i].name.start);
      printTab(tab + 2, "Expression:\n");
      showExpr(expr->expr_struct.properties[i].expr, tab + 4);
    }
  }

  else if (expr->kind == Expr_Array) {
    printTab(tab, "ArrayExpression\n");
    for (int i = 0; i < expr->expr_array.exprs_len; i++) {
      printTab(tab, "Element %d:\n", i);
      showExpr(expr->expr_array.exprs[i], tab + 2);
    }
  }
  
  else if (expr->kind == Expr_Literal) {
    printTab(tab, "LiteralExpression\n");
    printTab(tab, "Kind: %d\n", expr->expr_literal.kind);
    if (expr->expr_literal.kind == LITERAL_DOUBLE) printTab(tab, "Value: %f\n", expr->expr_literal.literal.numDouble);
    else if (expr->expr_literal.kind == LITERAL_FLOAT) printTab(tab, "Value: %f\n", expr->expr_literal.literal.numFloat);
    else if (expr->expr_literal.kind == LITERAL_INTEGER) printTab(tab, "Value: %ld\n", expr->expr_literal.literal.numInt);
    else if (expr->expr_literal.kind == LITERAL_UINTEGER) printTab(tab, "Value: %lu\n", expr->expr_literal.literal.numUint);
    else printTab(tab, "Value: %.*s\n", expr->expr_literal.literal.string.length, expr->expr_literal.literal.string.start);
  }

  else if (expr->kind == Expr_Identifier) {
    printTab(tab, "Identifier\n");
    printTab(tab, "Value: %.*s\n", expr->expr_identifier.length, expr->expr_identifier.start);
  }
}

void showNode(struct Node *node, int tab) {
  if (node == NULL) return printTab(tab, "(null)\n");

  else if (node->kind == Node_Import) {
    printTab(tab, "ImportNode\n");
    printTab(tab, "Import:\n");
    showExpr(node->node_import, tab + 2);
  }

  else if (node->kind == Node_Public) {
    printTab(tab, "PublicNode\n");
    printTab(tab, "Public:\n");
    showNode(node->node_public, tab + 2);
  }

  else if (node->kind == Node_Variable) {
    printTab(tab, "VariableNode\n");
    printTab(tab, "IsConst: %d\n", node->node_variable.isConst);
    for (int i = 0; i < node->node_variable.vars_len; i++) {
      printTab(tab, "Variable %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", node->node_variable.vars[i].name.length, node->node_variable.vars[i].name.start);
      printTab(tab + 2, "Type:\n");
      showType(node->node_variable.vars[i].type, tab + 4);
      printTab(tab + 2, "Expression:\n");
      showExpr(node->node_variable.vars[i].expr, tab + 4);
    }
  }

  else if (node->kind == Node_Function) {
    printTab(tab, "FunctionNode\n");
    printTab(tab, "Name: %.*s\n", node->node_function.name.length, node->node_function.name.start);
    printTab(tab, "Return Type:\n");
    showType(node->node_function.retType, tab + 2);
    for (int i = 0; i < node->node_function.params_len; i++) {
      printTab(tab, "Param %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", node->node_function.params[i].name.length, node->node_function.params[i].name.start);
      printTab(tab + 2, "Type:\n");
      showType(node->node_function.params[i].type, tab + 4);
    }
    printTab(tab, "Body:\n");
    showNode(node->node_function.body, tab + 2);
  }

  else if (node->kind == Node_Condition) {
    printTab(tab, "ConditionNode\n");
    printTab(tab, "Condition:\n");
    showExpr(node->node_condition.condition, tab + 2);
    printTab(tab, "trueBody:\n");
    showNode(node->node_condition.trueBody, tab + 2);
    printTab(tab, "falseBody:\n");
    showNode(node->node_condition.falseBody, tab + 2);
  }

  else if (node->kind == Node_LoopWhile) {
    printTab(tab, "LoopWhileNode\n");
    printTab(tab, "Condition:\n");
    showExpr(node->node_loopWhile.condition, tab + 2);
    printTab(tab, "DoWhile: %d\n", node->node_loopWhile.doWhile);
    printTab(tab, "body:\n");
    showNode(node->node_loopWhile.body, tab + 2);
  }

  else if (node->kind == Node_LoopFor) {
    printTab(tab, "LoopForNode\n");
    printTab(tab, "Init:\n");
    showNode(node->node_loopFor.init, tab + 2);
    printTab(tab, "Condition:\n");
    showExpr(node->node_loopFor.condition, tab + 2);
    printTab(tab, "Uptade:\n");
    showExpr(node->node_loopFor.uptade, tab + 2);
    printTab(tab, "body:\n");
    showNode(node->node_loopFor.body, tab + 2);
  }

  else if (node->kind == Node_Return) {
    printTab(tab, "ReturnNode\n");
    printTab(tab, "Return:\n");
    showExpr(node->node_return, tab + 2);
  }

  else if (node->kind == Node_Continue) printTab(tab, "ContinueNode\n");
  else if (node->kind == Node_Break) printTab(tab, "BreakNode\n");
  
  else if (node->kind == Node_Enum) {
    printTab(tab, "EnumNode\n");
    printTab(tab, "Name: %.*s\n", node->node_enum.name.length, node->node_enum.name.start);
    for (int i = 0; i < node->node_enum.elements_len; i++) {
      printTab(tab, "Element %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", node->node_enum.elements[i].name.length, node->node_enum.elements[i].name.start);
      printTab(tab + 2, "Expression:\n");
      showExpr(node->node_enum.elements[i].expr, tab + 4);
    }
  }

  else if (node->kind == Node_Struct) {
    printTab(tab, "StructNode\n");
    printTab(tab, "Name: %.*s\n", node->node_struct.name.length, node->node_struct.name.start);
    for (int i = 0; i < node->node_struct.properties_len; i++) {
      printTab(tab, "Property %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", node->node_struct.properties[i].name.length, node->node_struct.properties[i].name.start);
      printTab(tab + 2, "Type:\n");
      showType(node->node_struct.properties[i].type, tab + 4);
    }
    for (int i = 0; i < node->node_struct.methods_len; i++) {
      printTab(tab, "Method %d:\n", i);
      printTab(tab + 2, "Name: %.*s\n", node->node_struct.methods[i].name.length, node->node_struct.methods[i].name.start);
      printTab(tab + 2, "Return Type:\n");
      showType(node->node_struct.methods[i].retType, tab + 4);
      for (int j = 0; j < node->node_struct.methods[i].params_len; j++) {
        printTab(tab + 2, "Param %d:\n", i);
        printTab(tab + 4, "Name: %.*s\n", node->node_struct.methods[i].params[i].name.length, node->node_struct.methods[i].params[i].name.start);
        printTab(tab + 4, "Type:\n");
        showType(node->node_struct.methods[i].params[i].type, tab + 6);
      }
      printTab(tab + 2, "Body:\n");
      showNode(node->node_struct.methods[i].body, tab + 4);
    }
  }

  else if (node->kind == Node_Block) {
    printTab(tab, "BlockNode\n");
    for (int i = 0; i < node->node_block.nodes_len; i++) {
      printTab(tab, "Node %d:\n", i);
      showNode(node->node_block.nodes[i], tab + 2);
    }
  }

  else {
    printTab(tab, "ExpressionNode\n");
    printTab(tab, "Expression:\n");
    showExpr(node->node_expr, tab + 2);
  }
}

void showAst(struct Program *program) {
  printf("=============================== { AST } ===============================\n");
  for (int i = 0; i < program->length; i++) {
    printf(" %d:\n", i);
    showNode(program->nodes[i], 2);
  }
  printf("=============================== { AST } ===============================\n");
}
