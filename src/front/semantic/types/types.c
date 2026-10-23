#include "../semantic.h"
#include <stdio.h>
#include <utils/numeric.h>
#include <utils/error.h>

uint8_t flowNever(struct Flow *flow) {
  return !flow->next && !flow->return_ && !flow->break_ && !flow->continue_;
}

void typeFunc(struct SymbolTable *table, struct Decl *decl) {
  table->scope = decl->decl_function.scope;
  struct Flow flow = typeStmt(table, decl->decl_function.body);

  if (flow.next && !cmpTP(table->scope->expectType, Primitive_Void))
    errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value in all control paths");

  if (!flowNever(&flow) && table->scope->retType == NULL && !cmpTP(table->scope->expectType, Primitive_Void))
    errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value");

  if (decl->decl_function.retType->kind == Type_Auto) {
    struct Symbol *symbol = findSymbol(table->scope, &decl->decl_function.name);
    symbol->type->type_function.retType = table->scope->retType;
    decl->decl_function.retType = table->scope->retType;
  }

  exitScope(table);
}

void typeStruct(struct SymbolTable *table, struct Decl *decl) {
  for (int i = 0; i < decl->decl_struct.fields_len; i++) {
    if (decl->decl_struct.fields[i].type->kind != Type_Named) continue;
    struct Symbol *symbol = findSymbol(table->scope, &decl->decl_struct.fields[i].type->type_named.name);
    decl->decl_struct.fields[i].type->type_named.type = symbol->type;
  }

  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    table->scope = method->scope;
    struct Flow flow = typeStmt(table, method->body);

    if (flow.next && !cmpTP(table->scope->expectType, Primitive_Void))
      errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value in all control paths");

    if (!flowNever(&flow) && table->scope->retType == NULL && !cmpTP(table->scope->expectType, Primitive_Void))
      errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value");

    if (method->retType->kind == Type_Auto) {
      struct Symbol *s = hashmap_get(decl->decl_struct.symbol->symbol_struct.items, &method->name);
      s->type->type_function.retType = table->scope->retType;
      method->retType = table->scope->retType;
    }

    exitScope(table);
  }
}

void typeEnum(struct SymbolTable *table, struct Decl *decl) {
  for (int i = 0; i < decl->decl_enum.elems_len; i++) {
    table->scope->varType = NULL;
    struct Symbol *symbol = hashmap_get(decl->decl_enum.symbol->symbol_enum.items, &decl->decl_enum.elems[i].name);
    struct Type *type = typeExpr(table, decl->decl_enum.elems[i].expr);
    if (!isInteger(type))
      errorLang(table->program->filename, decl->line, decl->column, "enums can only contain integer types");
    symbol->type->type_named.type = type;
    decl->decl_enum.symbol->type = type;
  }
}

void resolveTypes(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];

    if (decl->kind == Decl_Public) {
      decl = decl->decl_public;
      if (decl->kind == Decl_Function) typeFunc(table, decl);
      else if (decl->kind == Decl_Struct) typeStruct(table, decl);
      else if (decl->kind == Decl_Enum) typeEnum(table, decl);
    }

    else if (decl->kind == Decl_Function) typeFunc(table, decl);
    else if (decl->kind == Decl_Struct) typeStruct(table, decl);
    else if (decl->kind == Decl_Enum) typeEnum(table, decl);
    typeDecl(table, decl);
  }
}
