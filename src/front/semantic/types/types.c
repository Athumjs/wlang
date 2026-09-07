#include "../semantic.h"
#include <utils/error.h>

uint8_t flowNever(struct Flow *flow) {
  return !flow->next && !flow->return_ && !flow->break_ && !flow->continue_;
}

void typeFunc(struct SymbolTable *table, struct Decl *decl) {
  struct Flow flow = typeStmt(table, decl->decl_function.body);

  if (flow.next)
    errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value in all control paths");

  if (!flowNever(&flow) && table->scope->retType == NULL && !cmpTP(table->scope->expectType, Primitive_Void))
    errorLang(table->program->filename, decl->line, decl->column, "non-void function does not return a value");
  exitScope(table);
}

void typeStruct(struct SymbolTable *table, struct Decl *decl) {
  struct Symbol *symbol = findSymbol(table->scope, &decl->decl_struct.name);
  for (int i = 0; i < decl->decl_struct.methods_len; i++) {
    struct Method *method = &decl->decl_struct.methods[i];
    table->scope->currentStruct = symbol;
    typeStmt(table, method->body);
    if (table->scope->retType == NULL && !cmpTP(table->scope->expectType, Primitive_Void))
      errorLang(table->program->filename, method->line, method->column, "non-void function does not return a value");
    exitScope(table);
  }
}

void resolveTypes(struct SymbolTable *table) {
  for (int i = 0; i < table->program->length; i++) {
    struct Decl *decl = table->program->decls[i];

    if (decl->kind == Decl_Public) {
      decl = decl->decl_public;
      if (decl->kind == Decl_Function) typeFunc(table, decl);
      else if (decl->kind == Decl_Struct) typeStruct(table, decl);
    }

    else if (decl->kind == Decl_Function) typeFunc(table, decl);
    else if (decl->kind == Decl_Struct) typeStruct(table, decl);
    typeDecl(table, decl);
  }
}
