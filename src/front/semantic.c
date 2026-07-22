#include <front/semantic.h>
#include <utils/hashmap.h>
#include <string.h>
#include <utils/error.h>

static inline uint8_t isNum(struct Type *type, uint8_t onlyIntegers) {
  if (onlyIntegers)
    return type->kind == Type_Primitive && (type->type_primitive.type == Primitive_Byte ||
        type->type_primitive.type == Primitive_Ubyte || type->type_primitive.type == Primitive_Short ||
        type->type_primitive.type == Primitive_Ushort || type->type_primitive.type == Primitive_Integer ||
        type->type_primitive.type == Primitive_Uinteger || type->type_primitive.type == Primitive_Long ||
        type->type_primitive.type == Primitive_Ulong);

  return type->kind == Type_Primitive && (type->type_primitive.type == Primitive_Byte ||
      type->type_primitive.type == Primitive_Ubyte || type->type_primitive.type == Primitive_Short ||
      type->type_primitive.type == Primitive_Ushort || type->type_primitive.type == Primitive_Integer ||
      type->type_primitive.type == Primitive_Uinteger || type->type_primitive.type == Primitive_Long ||
      type->type_primitive.type == Primitive_Ulong || type->type_primitive.type == Primitive_Float ||
      type->type_primitive.type == Primitive_Double);
}

struct Value {
  struct Type *type;
  struct Symbol *symbol;
};

struct Value semanticExpr(struct SymbolTable *table, struct Expr *expr) {
  struct Value value = {
    .type = NULL,
    .symbol = NULL
  };

  if (expr->kind == Expr_Assign) {
    struct Value left = semanticExpr(table, expr->expr_binary.left);
    struct Value right = semanticExpr(table, expr->expr_binary.right);

    if (left.symbol == NULL || left.symbol->kind != Symbol_Variable || (left.type != NULL && left.type->kind == Type_Array)) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expression is not assignable");
    }

    if (left.symbol->symbol_variable.isConst) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "cannot assign to '%.*s' because it is a constant",
          left.symbol->name.length, left.symbol->name.start);
    }

    if (left.type->kind == Type_Auto) left.type = right.type;

    if (!cmpType(table, left.type, right.type)) {
      struct String t1 = getType(right.type, table->program->arena);
      struct String t2 = getType(left.type, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
          t1.length, t1.start, t2.length, t2.start);
    }

    value.type = left.type;
    return value;
  }

  else if (expr->kind == Expr_Logical) {
    struct Value left = semanticExpr(table, expr->expr_binary.left);
    struct Value right = semanticExpr(table, expr->expr_binary.right);

    if (!cmpType(table, left.type, right.type) || left.type->kind != Type_Primitive || left.type->type_primitive.type != Primitive_Boolean) {
      struct String t1 = getType(left.type, table->program->arena);
      struct String t2 = getType(right.type, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
          tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
    }

    value.type = left.type;
    return value;
  }

  else if (expr->kind == Expr_Compare) {
    struct Value left = semanticExpr(table, expr->expr_binary.left);
    struct Value right = semanticExpr(table, expr->expr_binary.right);

    if (expr->expr_binary.op == TOKEN_GT || expr->expr_binary.op == TOKEN_GE ||
        expr->expr_binary.op == TOKEN_LT || expr->expr_binary.op == TOKEN_LE) {
      if (!isNum(left.type, 0) || !isNum(right.type, 0)) {
        struct String t1 = getType(left.type, table->program->arena);
        struct String t2 = getType(right.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
            tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
      }
    }

    else if (expr->expr_binary.op == TOKEN_EQ || expr->expr_binary.op == TOKEN_NE) {
      if ((!isNum(left.type, 0) && left.type->kind != Type_Pointer && (left.type->kind == Type_Primitive && left.type->type_primitive.type != Primitive_Boolean)) ||
          (!isNum(right.type, 0) && right.type->kind != Type_Pointer && (right.type->kind == Type_Primitive && right.type->type_primitive.type != Primitive_Boolean))) {
        struct String t1 = getType(left.type, table->program->arena);
        struct String t2 = getType(right.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
            tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
      }

      if (((left.type->kind == Type_Pointer || left.type->kind == Type_Array) ||
            (right.type->kind == Type_Pointer || right.type->kind == Type_Array)) && !cmpType(table, left.type, right.type)) {
        struct String t1 = getType(left.type, table->program->arena);
        struct String t2 = getType(right.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
            tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = arena_alloc(table->program->arena, sizeof(struct Type));
    value.type->kind = Type_Primitive;
    value.type->type_primitive.type = Primitive_Boolean;
    return value;
  }

  else if (expr->kind == Expr_Binary) {
    struct Value left = semanticExpr(table, expr->expr_binary.left);
    struct Value right = semanticExpr(table, expr->expr_binary.right);

    if (expr->expr_binary.op == TOKEN_PLUS || expr->expr_binary.op == TOKEN_MINUS ||
        expr->expr_binary.op == TOKEN_ASTERISK || expr->expr_binary.op == TOKEN_SLASH) {
      if (!isNum(left.type, 0) || !isNum(right.type, 0)) {
        struct String t1 = getType(left.type, table->program->arena);
        struct String t2 = getType(right.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
            tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
      }
    }

    else if (expr->expr_binary.op == TOKEN_MOD || expr->expr_binary.op == TOKEN_BIT_AND || expr->expr_binary.op == TOKEN_BIT_XOR ||
        expr->expr_binary.op == TOKEN_BIT_OR || expr->expr_binary.op == TOKEN_SHIFT_LEFT || expr->expr_binary.op == TOKEN_SHIFT_RIGHT) {
      if (!isNum(left.type, 1) || !isNum(right.type, 1)) {
        struct String t1 = getType(left.type, table->program->arena);
        struct String t2 = getType(right.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to types '%.*s' and '%.*s'",
            tk_names[expr->expr_binary.op], t1.length, t1.start, t2.length, t2.start);
      }
    }

    if ((left.type->kind == Type_Primitive && left.type->type_primitive.type == Primitive_Double) ||
        (right.type->kind == Type_Primitive && right.type->type_primitive.type == Primitive_Double)) {
      value.type = arena_alloc(table->program->arena, sizeof(struct Type));
      value.type->kind = Type_Primitive;
      value.type->type_primitive.type = Primitive_Double;
      return value;
    }

    else if ((left.type->kind == Type_Primitive && left.type->type_primitive.type == Primitive_Float) ||
        (right.type->kind == Type_Primitive && right.type->type_primitive.type == Primitive_Float)) {
      value.type = arena_alloc(table->program->arena, sizeof(struct Type));
      value.type->kind = Type_Primitive;
      value.type->type_primitive.type = Primitive_Float;
      return value;
    } 

    value.type = left.type;
    return value;
  }

  else if (expr->kind == Expr_Cast) {
    struct Value v = semanticExpr(table, expr->expr_cast.value);
    resolveType(table, &expr->expr_cast.type);

    if (!cmpType(table, v.type, expr->expr_cast.type)) {
      if ((v.type->kind != Type_Primitive && v.type->kind != Type_Pointer) ||
          (expr->expr_cast.type->kind != Type_Primitive && expr->expr_cast.type->kind != Type_Pointer)) {
        struct String t1 = getType(v.type, table->program->arena);
        struct String t2 = getType(expr->expr_cast.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "cannot cast type '%.*s' to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }

      if ((!isNum(v.type, 0) && v.type->kind != Type_Pointer) || (!isNum(expr->expr_cast.type, 0) && expr->expr_cast.type->kind != Type_Pointer)) {
        struct String t1 = getType(v.type, table->program->arena);
        struct String t2 = getType(expr->expr_cast.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "cannot cast type '%.*s' to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = expr->expr_cast.type;
    return value;
  }

  else if (expr->kind == Expr_Unary) {
    struct Value arg = semanticExpr(table, expr->expr_unary.arg);

    if (expr->expr_unary.op == TOKEN_NOT) {
      if (arg.type->kind != Type_Primitive || arg.type->type_primitive.type != Primitive_Boolean) {
        struct String t = getType(arg.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '!' cannot be applied to type '%.*s'",
            t.length, t.start);
      }
    }

    else if (expr->expr_unary.op == TOKEN_BIT_NOT) {
      if (!isNum(arg.type, 1)) {
        struct String t = getType(arg.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '~' cannot be applied to type '%.*s'",
            t.length, t.start);
      }
    }

    else if (expr->expr_unary.op == TOKEN_MINUS) {
      if (!isNum(arg.type, 0)) {
        struct String t = getType(arg.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '-' cannot be applied to type '%.*s'",
            t.length, t.start);
      }
    }

    else if (expr->expr_unary.op == TOKEN_BIT_AND) {
      if (arg.symbol == NULL) {
        errorLang(table->program->args->input_file, expr->line, expr->column, "cannot take the address of a literal");
      }

      struct Type *type = arena_alloc(table->program->arena, sizeof(struct Type));
      type->kind = Type_Pointer;
      type->type_pointer.base = arg.type;
      value.type = type;
      return value;
    }

    else if (expr->expr_unary.op == TOKEN_INCREMENT || expr->expr_unary.op == TOKEN_DECREMENT) {
      if (arg.symbol == NULL) {
        errorLang(table->program->args->input_file, expr->line, expr->column, "expression is not assignable");
      }

      if (!isNum(arg.type, 0) && arg.type->kind != Type_Pointer) {
        struct String t = getType(arg.type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "operator '%s' cannot be applied to type '%.*s'",
            tk_names[expr->expr_unary.op], t.length, t.start);
      }
    }

    value.type = arg.type;
    return value;
  }

  else if (expr->kind == Expr_Index) {
    struct Value base = semanticExpr(table, expr->expr_index.base);

    if (base.type->kind != Type_Array && base.type->kind != Type_Pointer) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "subscripted value is not an array or pointer");
    }

    struct Value index = semanticExpr(table, expr->expr_index.index);

    if (!isNum(index.type, 1)) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "array subscript is not a integer");
    }

    if (base.type->kind == Type_Array) {
      size_t i;
      arrayIsConstant(table, expr->expr_index.index, &i);

      if (i && base.type->type_array.length && i >= base.type->type_array.length) {
        errorLang(table->program->args->input_file, expr->line, expr->column, "index %d is outside the bounds of array (size %d)",
            i, base.type->type_array.length);
      }
      value.type = base.type->type_array.base;
    }

    else value.type = base.type->type_pointer.base;
    value.symbol = base.symbol;
    return value;
  }

  else if (expr->kind == Expr_Member) {
    struct Value obj = semanticExpr(table, expr->expr_member.obj);

    if (obj.symbol == NULL || (obj.symbol->kind != Symbol_Variable && obj.symbol->kind != Symbol_Enum)) {
      struct String t = getType(obj.type, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "member reference base type '%.*s' is not a structure",
          t.length, t.start);
    }

    struct Symbol *member = NULL;
    if (obj.symbol->kind == Symbol_Variable) {
      struct Symbol *symbol = findSymbol(table->scope, &obj.symbol->type->type_named.name);
      member = hashmap_get(symbol->symbol_struct.items, &expr->expr_member.member->expr_identifier);
    }
    else member = hashmap_get(obj.symbol->symbol_enum.items, &expr->expr_member.member->expr_identifier);

    if (member == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
          expr->expr_member.member->expr_identifier.length, expr->expr_member.member->expr_identifier.start, obj.symbol->name.length, obj.symbol->name.start);
    }

    value.type = member->type;
    value.symbol = member;
    return value;
  }

  else if (expr->kind == Expr_Call) {
    struct Value callee = semanticExpr(table, expr->expr_call.callee);

    if (callee.symbol == NULL || callee.type->kind != Type_Function) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "this expression is not callable");
    }

    if (expr->expr_call.args_len != callee.type->type_function.params_len) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected %d arguments, but got %d",
          callee.symbol->symbol_function.params_len, expr->expr_call.args_len);
    }

    for (int i = 0; i < callee.type->type_function.params_len; i++) {
      struct Type *param = callee.type->type_function.params[i];
      struct Value arg = semanticExpr(table, expr->expr_call.args[i]);
      if (param->kind == Type_Auto) param = arg.type;

      if (!cmpType(table, arg.type, param)) {
        struct String t1 = getType(arg.type, table->program->arena);
        struct String t2 = getType(param, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = callee.type->type_function.retType;
    value.symbol = callee.symbol;
    return value;
  }

  else if (expr->kind == Expr_Struct) {
    if (table->scope->expectType->kind == Type_Auto) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "cannot infer type of struct literal");
    }

    if (table->scope->expectType->kind != Type_Named) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected expression");
    }

    struct Symbol *type = findSymbol(table->scope, &table->scope->expectType->type_named.name);

    if (type == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'", 
          table->scope->expectType->type_named.name.length, table->scope->expectType->type_named.name.start);
    }

    if (type->kind != Symbol_Struct) {
      struct String str = getType(table->scope->expectType, table->program->arena);
      errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
    }

    for (int i = 0; i < expr->expr_struct.properties_len; i++) {
      struct Property *p1 = &expr->expr_struct.properties[i];
      struct Symbol *p2 = hashmap_get(type->symbol_struct.items, &p1->name);

      if (!p2) {
        errorLang(table->program->args->input_file, expr->line, expr->column, "cannot find name '%.*s'",
            p1->name.length, p1->name.start);
      }

      struct Type *old = table->scope->expectType;
      table->scope->expectType = p2->type;
      struct Value v = semanticExpr(table, p1->expr);
      table->scope->expectType = old;

      if (!cmpType(table, p2->type, v.type)) {
        struct String t1 = getType(v.type, table->program->arena);
        struct String t2 = getType(p2->type, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            t1.length, t1.start, t2.length, t2.start);
      }
    }

    value.type = table->scope->expectType;
    return value;
  }

  else if (expr->kind == Expr_Array) {
    value.type = arena_alloc(table->program->arena, sizeof(struct Type));
    value.type->kind = Type_Array;
    value.type->type_array.length = expr->expr_array.exprs_len;
    
    if (table->scope->expectType->kind == Type_Auto) {
      if (expr->expr_array.exprs_len == 0) value.type->type_array.base = NULL;
      else value.type->type_array.base = semanticExpr(table, expr->expr_array.exprs[0]).type; 
    } else {
      if (table->scope->expectType->kind != Type_Array) {
        struct String str = getType(table->scope->expectType, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "expected '%.*s'", str.length, str.start);
      }

      value.type->type_array.base = table->scope->expectType->type_array.base;
    }

    for (int i = 0; i < expr->expr_array.exprs_len; i++) {
      struct Type *old = table->scope->expectType;
      table->scope->expectType = value.type->type_array.base;
      struct Value exprValue = semanticExpr(table, expr->expr_array.exprs[i]);
      table->scope->expectType = old;

      if (!cmpType(table, exprValue.type, value.type->type_array.base)) {
        struct String s1 = getType(exprValue.type, table->program->arena);
        struct String s2 = getType(value.type->type_array.base, table->program->arena);
        errorLang(table->program->args->input_file, expr->line, expr->column, "type '%.*s' is not assignable to type '%.*s'",
            s1.length, s1.start, s2.length, s2.start);
      }
    }

    return value;
  }

  else if (expr->kind == Expr_Literal) {
    value.type = arena_alloc(table->program->arena, sizeof(struct Type));
    value.type->kind = Type_Primitive;

    if (expr->expr_literal.kind == LITERAL_INTEGER) {
      value.type->type_primitive.type = Primitive_Integer;
    }

    else if (expr->expr_literal.kind == LITERAL_UINTEGER) {
      value.type->type_primitive.type = Primitive_Uinteger;
    }

    else if (expr->expr_literal.kind == LITERAL_FLOAT) {
      value.type->type_primitive.type = Primitive_Float;
    }

    else if (expr->expr_literal.kind == LITERAL_DOUBLE) {
      value.type->type_primitive.type = Primitive_Double;
    }

    else if (expr->expr_literal.kind == LITERAL_CHAR) {
      value.type->type_primitive.type = Primitive_Char;
    }

    else if (expr->expr_literal.kind == LITERAL_STRING) {
      value.type->type_primitive.type = Primitive_Char;
      struct Type *temp = arena_alloc(table->program->arena, sizeof(struct Type));
      temp->kind = Type_Pointer;
      temp->type_pointer.base = value.type;
      value.type = temp;
    }

    else if (expr->expr_literal.kind == LITERAL_BOOLEAN) {
      value.type->type_primitive.type = Primitive_Boolean;
    }

    return value;
  }

  else if (expr->kind == Expr_This) {
    if (table->scope->currentStruct == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "'this' outside of struct method");
    }

    struct Symbol *member = hashmap_get(table->scope->currentStruct->symbol_struct.items, &expr->expr_this);

    if (member == NULL) {
      errorLang(table->program->args->input_file, expr->line, expr->column, "no member named '%.*s' in '%.*s'",
          expr->expr_this.length, expr->expr_this.start, table->scope->currentStruct->name.length, table->scope->currentStruct->name.start);
    }

    value.type = member->type;
    value.symbol = member;
    return value;
  }

  struct Symbol *symbol = findSymbol(table->scope, &expr->expr_identifier);

  if (symbol == NULL) {
    errorLang(table->program->args->input_file, expr->line, expr->column, "Cannot find name '%.*s'", expr->expr_identifier.length, expr->expr_identifier.start);
  }

  value.type = symbol->type;
  value.symbol = symbol;
  return value;
}

void semanticNode(struct SymbolTable *table, struct Node *node) {
  if (node->kind == Node_Variable) {
    for (int i = 0; i < node->node_variable.vars_len; i++) {
      struct Var *var = &node->node_variable.vars[i];
      resolveType(table, &var->type);
      struct Type *old = table->scope->expectType;
      table->scope->expectType = var->type;

      if (node->node_variable.isConst && var->expr == NULL) {
        errorLang(table->program->args->input_file, node->line, node->column, "'const' declarations must be initialized");
      }

      if (var->expr != NULL) {
        struct Value expr = semanticExpr(table, var->expr);
        if (var->type->kind == Type_Auto) var->type = expr.type;

        if (!cmpType(table, var->type, expr.type)) {
          struct String t1 = getType(expr.type, table->program->arena);
          struct String t2 = getType(var->type, table->program->arena);
          errorLang(table->program->args->input_file, node->line, node->column, "type '%.*s' is not assignable to type '%.*s'",
              t1.length, t1.start, t2.length, t2.start);
        }
      }

      struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
      symbol->kind = Symbol_Variable;
      symbol->line = node->line;
      symbol->column = node->column;
      symbol->name = var->name;
      symbol->type = var->type;
      symbol->symbol_variable.isConst = node->node_variable.isConst;
      addSymbol(table, symbol);
      table->scope->expectType = old;
    }
  }

  else if (node->kind == Node_Function) {
    resolveType(table, &node->node_function.retType); 
    enterScope(table);
    table->scope->expectType = node->node_function.retType;

    for (int i = 0; i < node->node_function.params_len; i++) {
      struct Label *param = &node->node_function.params[i];
      resolveType(table, &param->type);

      struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
      symbol->kind = Symbol_Variable;
      symbol->line = param->line;
      symbol->column = param->column;
      symbol->name = param->name;
      symbol->type = param->type;
      symbol->symbol_variable.isConst = 0;
      addSymbol(table, symbol);
    }

    semanticNode(table, node->node_function.body);
    if (node->node_function.retType != NULL && table->scope->retType == NULL && node->node_function.retType->kind == Type_Primitive &&
        node->node_function.retType->type_primitive.type != Primitive_Void) {
      errorLang(table->program->args->input_file, node->line, node->column, "non-void function does not return a value");
    }
    exitScope(table);
  }

  else if (node->kind == Node_Condition) {
    semanticExpr(table, node->node_condition.condition);
    enterScope(table);
    semanticNode(table, node->node_condition.trueBody);
    exitScope(table);
    if (node->node_condition.falseBody != NULL) {
      enterScope(table);
      semanticNode(table, node->node_condition.falseBody);
      exitScope(table);
    }
  }

  else if (node->kind == Node_LoopWhile) {
    semanticExpr(table, node->node_loopWhile.condition);
    enterScope(table);
    table->scope->onLoop = 1;
    semanticNode(table, node->node_loopWhile.body);
    exitScope(table);
  }

  else if (node->kind == Node_LoopFor) {
    enterScope(table);
    table->scope->onLoop = 1;
    semanticNode(table, node->node_loopFor.init);
    semanticExpr(table, node->node_loopFor.condition);
    semanticExpr(table, node->node_loopFor.uptade);
    semanticNode(table, node->node_loopFor.body);
    exitScope(table);
  }

  else if (node->kind == Node_Return) {
    if (node->node_return != NULL) table->scope->retType = semanticExpr(table, node->node_return).type;
    else {
      table->scope->retType = arena_alloc(table->program->arena, sizeof(struct Type));
      table->scope->retType->kind = Type_Primitive;
      table->scope->retType->type_primitive.type = Primitive_Void;
    }

    if (table->scope->expectType->kind == Type_Auto) {
      table->scope->expectType = table->scope->retType;
    }

    if (!cmpType(table, table->scope->expectType, table->scope->retType)) {
      struct String t1 = getType(table->scope->expectType, table->program->arena);
      struct String t2 = getType(table->scope->retType, table->program->arena);
      errorLang(table->program->args->input_file, node->line, node->column, "type '%.*s' is not assignable to type '%.*s'",
          t1.length, t1.start, t2.length, t2.start);
    }
  }

  else if (node->kind == Node_Continue) {
    if (!table->scope->onLoop) {
      errorLang(table->program->args->input_file, node->line, node->column, "invalid continue statement");
    }
  }

  else if (node->kind == Node_Break) {
    if (!table->scope->onLoop) {
      errorLang(table->program->args->input_file, node->line, node->column, "invalid break statement");
    }
  }

  else if (node->kind == Node_Enum) {}

  else if (node->kind == Node_Struct) {
    struct Symbol *symbol = findSymbol(table->scope, &node->node_struct.name);
    for (int i = 0; i < node->node_struct.methods_len; i++) {
      struct Method *method = &node->node_struct.methods[i];
      resolveType(table, &method->retType);
      enterScope(table);
      table->scope->expectType = method->retType;
      table->scope->currentStruct = symbol;

      for (int i = 0; i < method->params_len; i++) {
        struct Label *param = &method->params[i];
        resolveType(table, &param->type);
	
        struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
        symbol->kind = Symbol_Variable;
        symbol->line = param->line;
        symbol->column = param->column;
        symbol->name = param->name;
        symbol->type = param->type;
        symbol->symbol_variable.isConst = 1;
        addSymbol(table, symbol);
      }
	
      semanticNode(table, method->body);
      if (method->retType != NULL && table->scope->retType == NULL && method->retType->kind == Type_Primitive &&
          method->retType->type_primitive.type != Primitive_Void) {
        errorLang(table->program->args->input_file, node->line, node->column, "non-void function does not return a value");
      }
      exitScope(table);
    }
  }

  else if (node->kind == Node_Block) {
    for (int i = 0; i < node->node_block.nodes_len; i++) {
      semanticNode(table, node->node_block.nodes[i]);
    }
  }

  else semanticExpr(table, node->node_expr);
}

void funcSymbol(struct SymbolTable *table, struct Node *node) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = Symbol_Function;
  symbol->line = node->line;
  symbol->column = node->column;
  symbol->name = node->node_function.name;
  symbol->type = arena_alloc(table->program->arena, sizeof(struct Type));
  symbol->type->kind = Type_Function;
  symbol->type->type_function.retType = node->node_function.retType;
  symbol->type->type_function.params_cap = node->node_function.params_cap;
  symbol->type->type_function.params = arena_alloc(table->program->arena, node->node_function.params_cap * sizeof(struct Type *));
  symbol->type->type_function.params_len = node->node_function.params_len;
  for (int i = 0; i < node->node_function.params_len; i++) {
    resolveType(table, &node->node_function.params[i].type);
    symbol->type->type_function.params[i] = node->node_function.params[i].type;
  }
  symbol->symbol_function.params = node->node_function.params;
  symbol->symbol_function.params_len = node->node_function.params_len;
  addSymbol(table, symbol);
}

void enumSymbol(struct SymbolTable *table, struct Node *node) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = Symbol_Enum;
  symbol->line = node->line;
  symbol->column = node->column;
  symbol->name = node->node_enum.name;
  symbol->type = arena_alloc(table->program->arena, sizeof(struct Type));
  symbol->type->kind = Type_Named;
  symbol->type->type_named.name = node->node_enum.name;
  symbol->symbol_enum.items = hashmap_new(table->program->arena, 8);
  for (int i = 0; i < node->node_enum.elements_len; i++) {
    struct Element *element = &node->node_enum.elements[i];
    struct Symbol *elementSymbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
    elementSymbol->kind = Symbol_EnumValue;
    elementSymbol->line = element->line;
    elementSymbol->column = element->column;
    elementSymbol->name = element->name;
    elementSymbol->type = symbol->type;
    hashmap_set(symbol->symbol_enum.items, &element->name, elementSymbol, table->program->arena);
  }
  addSymbol(table, symbol);
}

void structSymbol(struct SymbolTable *table, struct Node *node) {
  struct Symbol *symbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
  symbol->kind = Symbol_Struct;
  symbol->line = node->line;
  symbol->column = node->column;
  symbol->name = node->node_struct.name;
  symbol->type = arena_alloc(table->program->arena, sizeof(struct Type));
  symbol->type->kind = Type_Named;
  symbol->type->type_named.name = node->node_struct.name;
  symbol->symbol_struct.items = hashmap_new(table->program->arena, 8);
  for (int i = 0; i < node->node_struct.properties_len; i++) {
    struct Label *prop = &node->node_struct.properties[i];
    resolveType(table, &prop->type);
    struct Symbol *propSymbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
    propSymbol->kind = Symbol_Property;
    propSymbol->line = prop->line;
    propSymbol->column = prop->column;
    propSymbol->name = prop->name;
    propSymbol->type = prop->type;
    hashmap_set(symbol->symbol_struct.items, &prop->name, propSymbol, table->program->arena);
  }
  for (int i = 0; i < node->node_struct.methods_len; i++) {
    struct Method *method = &node->node_struct.methods[i];
    resolveType(table, &method->retType);
    struct Symbol *methodSymbol = arena_alloc(table->program->arena, sizeof(struct Symbol));
    methodSymbol->kind = Symbol_Method;
    methodSymbol->line = method->line;
    methodSymbol->column = method->column;
    methodSymbol->name = method->name;
    methodSymbol->type = arena_alloc(table->program->arena, sizeof(struct Type));
    methodSymbol->type->kind = Type_Function;
    methodSymbol->type->type_function.retType = method->retType;
    methodSymbol->type->type_function.params_cap = method->params_cap;
    methodSymbol->type->type_function.params = arena_alloc(table->program->arena, method->params_cap * sizeof(struct Type *));
    methodSymbol->type->type_function.params_len = method->params_len;
    for (int i = 0; i < method->params_len; i++) {
      resolveType(table, &method->params[i].type);
      methodSymbol->type->type_function.params[i] = method->params[i].type;
    }
    methodSymbol->symbol_function.params = method->params;
    methodSymbol->symbol_function.params_len = method->params_len;
    hashmap_set(symbol->symbol_struct.items, &method->name, methodSymbol, table->program->arena);
  }
  addSymbol(table, symbol);
}

void semantic(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Node *node = table->program->nodes[i];
    if (node->kind == Node_Function) funcSymbol(table, node);
    else if (node->kind == Node_Enum) enumSymbol(table, node);
    else if (node->kind == Node_Struct) structSymbol(table, node);
    continue;
  }

  for (int i = 0; i < table->program->length; i++) {
    semanticNode(table, table->program->nodes[i]);
  }
}
