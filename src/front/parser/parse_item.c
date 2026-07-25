#include "parser.h"

struct Item parseItem(int *i, struct Tokens *tokens, struct Program *program) {
  struct Decl *decl = parseDecl(i, tokens, program);

  if (decl == NULL) return (struct Item) {
    .kind = Item_Stmt,
    .item_stmt = parseStmt(i, tokens, program)
  };

  return (struct Item) {
    .kind = Item_Decl,
    .item_decl = decl
  };
}
