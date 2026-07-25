#include "parser.h"
#include <string.h>

struct Decl *declImport(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Import);
  CONSUME(TOKEN_IMPORT);
  decl->decl_import = parseExpr(i, tokens, program);
  CONSUME(TOKEN_SEMICOLON);
  return decl;
}

struct Decl *declPublic(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Public);
  CONSUME(TOKEN_PUBLIC);
  decl->decl_public = parseDecl(i, tokens, program);
  return decl;
}

struct Decl *declVariable(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Variable);
  decl->decl_variable.vars_cap = 2;
  decl->decl_variable.vars = arena_alloc(program->arena, decl->decl_variable.vars_cap * sizeof(struct Var));

  if (PEEK() == TOKEN_CONST) {
    CONSUME(TOKEN_CONST);
    decl->decl_variable.isConst = 1;
  } else CONSUME(TOKEN_VAR);

  while (1) {
    if (decl->decl_variable.vars_len == decl->decl_variable.vars_cap) {
      size_t oldCap = decl->decl_variable.vars_cap;
      decl->decl_variable.vars_cap *= 2;
      struct Var *temp = arena_alloc(program->arena, decl->decl_variable.vars_cap * sizeof(struct Var));
      memcpy(temp, decl->decl_variable.vars, oldCap * sizeof(struct Var));
      decl->decl_variable.vars = temp;
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

    decl->decl_variable.vars[decl->decl_variable.vars_len++] = var;
    if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
    else break;
  }

  CONSUME(TOKEN_SEMICOLON);
  return decl;
}

struct Decl *declFunction(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Function);
  CONSUME(TOKEN_SET);
  decl->decl_function.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LPAREN);

  if (PEEK() != TOKEN_RPAREN) {
    decl->decl_function.params_cap = 2;
    decl->decl_function.params = arena_alloc(program->arena, decl->decl_function.params_cap * sizeof(struct Param));

    while (1) {
      if (decl->decl_function.params_len == decl->decl_function.params_cap) {
        size_t oldCap = decl->decl_function.params_cap;
        decl->decl_function.params_cap *= 2;
        struct Param *temp = arena_alloc(program->arena, decl->decl_function.params_cap * sizeof(struct Param));
        memcpy(temp, decl->decl_function.params, oldCap * sizeof(struct Param));
        decl->decl_function.params = temp;
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

      decl->decl_function.params[decl->decl_function.params_len++] = param;
      if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
      else break;
    }
  }

  CONSUME(TOKEN_RPAREN);
  
  if (PEEK() == TOKEN_COLON) {
    CONSUME(TOKEN_COLON);
    decl->decl_function.retType = parseType(i, tokens, program);
  }

  decl->decl_function.body = parseStmt(i, tokens, program);
  return decl;
}

struct Decl *declEnum(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Enum);
  CONSUME(TOKEN_ENUM);
  decl->decl_enum.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    decl->decl_enum.elems_cap = 2;
    decl->decl_enum.elems = arena_alloc(program->arena, decl->decl_enum.elems_cap * sizeof(struct Element));

    while (1) {
      if (decl->decl_enum.elems_len == decl->decl_enum.elems_cap) {
        size_t oldCap = decl->decl_enum.elems_cap;
        decl->decl_enum.elems_cap *= 2;
        struct Element *temp = arena_alloc(program->arena, decl->decl_enum.elems_cap * sizeof(struct Element));
        memcpy(temp, decl->decl_enum.elems, oldCap * sizeof(struct Element));
        decl->decl_enum.elems = temp;
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

      decl->decl_enum.elems[decl->decl_enum.elems_len++] = element;

      if (PEEK() == TOKEN_COMMA) CONSUME(TOKEN_COMMA);
      else break;
    }
  }

  CONSUME(TOKEN_RBRACE);
  if (PEEK() == TOKEN_SEMICOLON) CONSUME(TOKEN_SEMICOLON);
  return decl;
}

void resolveStructMethods(int *i, struct Tokens *tokens, struct Program *program, struct Decl *decl) {
	if (decl->decl_struct.methods_len == decl->decl_struct.fields_cap) {
	  size_t oldCap = decl->decl_struct.methods_cap;
	  decl->decl_struct.methods_cap *= 2;
	  struct Method *temp = arena_alloc(program->arena, decl->decl_struct.methods_cap * sizeof(struct Field));
	  memcpy(temp, decl->decl_struct.methods, oldCap * sizeof(struct Method));
	  decl->decl_struct.methods = temp;
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
	  method.params = arena_alloc(program->arena, method.params_cap * sizeof(struct Param));
	
	  while (1) {
	    if (method.params_len == method.params_cap) {
	      size_t oldCap = method.params_cap;
	      method.params_cap *= 2;
	      struct Param *temp = arena_alloc(program->arena, method.params_cap * sizeof(struct Param));
	      memcpy(temp, method.params, oldCap * sizeof(struct Param));
	      method.params = temp;
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

	method.body = parseStmt(i, tokens, program);
	decl->decl_struct.methods[decl->decl_struct.methods_len++] = method;
  if (PEEK() != TOKEN_RBRACE) resolveStructMethods(i, tokens, program, decl);
}

void resolveStructProperties(int *i, struct Tokens *tokens, struct Program *program, struct Decl *decl) {
  if (PEEK() == TOKEN_SET) {
    decl->decl_struct.methods_cap = 2;
    decl->decl_struct.methods = arena_alloc(program->arena, decl->decl_struct.methods_cap * sizeof(struct Method));
    return resolveStructMethods(i, tokens, program, decl);
  }

	if (decl->decl_struct.fields_len == decl->decl_struct.fields_cap) {
	  size_t oldCap = decl->decl_struct.fields_cap;
	  decl->decl_struct.fields_cap *= 2;
	  struct Field *temp = arena_alloc(program->arena, decl->decl_struct.fields_cap * sizeof(struct Field));
	  memcpy(temp, decl->decl_struct.fields, oldCap * sizeof(struct Param));
	  decl->decl_struct.fields = temp;
  }
	
	struct Field field = {
	  .line = tokens->token[*i].line,
	  .column = tokens->token[*i].column,
	  .name = CONSUME(IDENTIFIER).string,
    .type = NULL
	};

  CONSUME(TOKEN_COLON);
  field.type = parseType(i, tokens, program);
	
	decl->decl_struct.fields[decl->decl_struct.fields_len++] = field;
  CONSUME(TOKEN_SEMICOLON);
	if (PEEK() != TOKEN_RBRACE) resolveStructProperties(i, tokens, program, decl);
}

struct Decl *declStruct(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = newDecl(i, tokens, program, Decl_Struct);
  CONSUME(TOKEN_STRUCT);
  decl->decl_struct.name = CONSUME(IDENTIFIER).string;
  CONSUME(TOKEN_LBRACE);

  if (PEEK() != TOKEN_RBRACE) {
    decl->decl_struct.fields_cap = 2;
    decl->decl_struct.fields = arena_alloc(program->arena, decl->decl_struct.fields_cap * sizeof(struct Field));
    resolveStructProperties(i, tokens, program, decl);
  }

  CONSUME(TOKEN_RBRACE);
  if (PEEK() == TOKEN_SEMICOLON) CONSUME(TOKEN_SEMICOLON);
  return decl;
}

struct Decl *parseDecl(int *i, struct Tokens *tokens, struct Program *program) {
  if (PEEK() == TOKEN_IMPORT) return declImport(i, tokens, program);
  else if (PEEK() == TOKEN_PUBLIC) return declPublic(i, tokens, program);
  else if (PEEK() == TOKEN_CONST || PEEK() == TOKEN_VAR) return declVariable(i, tokens, program);
  else if (PEEK() == TOKEN_SET) return declFunction(i, tokens, program);
  else if (PEEK() == TOKEN_ENUM) return declEnum(i, tokens, program);
  else if (PEEK() == TOKEN_STRUCT) return declStruct(i, tokens, program);

  return NULL;
}
