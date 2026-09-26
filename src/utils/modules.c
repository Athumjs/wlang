#include <utils/modules.h>
#include <front/semantic.h>
#include <middle/irgen.h>
#include <utils/error.h>
#include <stdio.h>

void printOpcode(enum IROpcode opcode) {
#define X(name, str) \
  if (opcode == name) \
    printf("%s ", str);
  OPCODES
#undef X
}

void printValue(struct IRValue *value) {
  if (value->kind == Value_Int)
    printf("%ld", value->i);
  else if (value->kind == Value_UInt)
    printf("%ldu", value->u);
  else if (value->kind == Value_Float)
    printf("%ff", value->f);
  else if (value->kind == Value_Double)
    printf("%f", value->d);
  else if (value->kind == Value_String)
    printf("%.*s", value->s->length, value->s->start);
}

void printOperand(struct IROperand *op, struct Arena *arena);

void printInst(struct IRInstruction *inst, struct Arena *arena) { 
  if (inst->vl != -1)
    printf("%%%ld = ", inst->vl);

  enum IROpcode opcode = inst->opcode;
  printOpcode(opcode);

  if (inst->type != NULL) {
    struct String type = getType(inst->type, arena);
    printf("%.*s ", type.length, type.start);
  }
  

  for (int l = 0; l < inst->operands_len; l++) {
    if (l > 0) printf(", ");
    struct IROperand *op = &inst->operands[l];
    printOperand(op, arena);
  }

  printf("\n    ");
}

void printOperand(struct IROperand *op, struct Arena *arena) {
  if (op->kind == Operand_Global)
    printf("@%ld", op->ref);
  else if (op->kind == Operand_Register)
    printf("%%%ld", op->ref);
  else if (op->kind == Operand_Block)
    printf("b%ld", op->ref);
  else if (op->kind == Operand_Constant)
    printValue(&op->constant);

  else if (op->kind == Operand_Cmp && op->cmp.kind == CMP_INT) {
    if (op->cmp.icmp == ICMP_Eq)
      printf("eq");
    else if (op->cmp.icmp == ICMP_Ne)
      printf("ne");
    else if (op->cmp.icmp == ICMP_SGt)
      printf("sgt");
    else if (op->cmp.icmp == ICMP_SGe)
      printf("sge");
    else if (op->cmp.icmp == ICMP_SLt)
      printf("slt");
    else if (op->cmp.icmp == ICMP_SLe)
      printf("sle");
    else if (op->cmp.icmp == ICMP_UGt)
      printf("ugt");
    else if (op->cmp.icmp == ICMP_UGe)
      printf("uge");
    else if (op->cmp.icmp == ICMP_ULt)
      printf("ult");
    else if (op->cmp.icmp == ICMP_ULe)
      printf("ule");
  }

  else if (op->kind == Operand_Cmp && op->cmp.kind == CMP_FLOAT) {
    if (op->cmp.fcmp == FCMP_OEq)
      printf("oeq");
    else if (op->cmp.fcmp == FCMP_ONe)
      printf("one");
    else if (op->cmp.fcmp == FCMP_OGt)
      printf("ogt");
    else if (op->cmp.fcmp == FCMP_OGe)
      printf("oge");
    else if (op->cmp.fcmp == FCMP_OLt)
      printf("olt");
    else if (op->cmp.fcmp == FCMP_OLe)
      printf("ole");
  }

  else if (op->kind == Operand_Phi) {
    if (op->phi.block->index == 0) printf("[ %%%ld, entry ]", op->phi.value);
    else printf("[ %%%ld, b%ld ]", op->phi.value, op->phi.block->index);
  }
}

void showIR(struct IRModule *ir, struct Arena *arena) {
  printf("IR\n\n");
  for (int i = 0; i < ir->globals_len; i++) {
    struct String type = getType(ir->globals[i].type, arena);
    printf("@%ld = global %.*s ", ir->globals[i].vl, type.length, type.start);
    printOperand(&ir->globals[i].value, arena);
    printf("\n");
  }

  if (ir->init.blocks[0].instructions_len > 0) printf("\n#init:\n    ");
  for (int i = 0; i < ir->init.blocks[0].instructions_len; i++)
    printInst(&ir->init.blocks[0].instructions[i], arena);

  printf("\n");
  for (int i = 0; i < ir->functions_len; i++) {
    printf("define %.*s(", ir->functions[i].name->length, ir->functions[i].name->start);
    for (int j = 0; j < ir->functions[i].params_len; j++) {
      if (j > 0) printf(", ");
      struct String type = getType(ir->functions[i].params[j].type, arena);
      printf("%.*s %%%d", type.length, type.start, ir->functions[i].params[j].vl);
    }

    printf(")\n  ");
    for (int j = 0; j < ir->functions[i].blocks_len; j++) {
      if (j == 0) printf("entry:\n    ");
      else printf("b%d:\n    ", j); 

      for (int k = 0; k < ir->functions[i].blocks[j].instructions_len; k++)
        printInst(&ir->functions[i].blocks[j].instructions[k], arena);

      printf("\n  ");
    }
    printf("\n");
  }
}

struct Hashmap *load_module(struct Module *modules, struct Arena *arena, char *path) {
  FILE *file = fopen(path, "r");
  if (file == NULL) errorGeneric("'%s' no such file or directory", path);

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  char *code = arena_alloc(arena, size + 1);
  fread(code, 1, size, file);
  code[size] = '\0';
  fclose(file);

  struct Tokens tokens = {0};
  tokens.capacity = 256;
  tokens.token = arena_alloc(arena, tokens.capacity * sizeof(struct Token));
  tokens.length = 0;

  struct Program program = {0};
  program.filename = path;
  program.arena = arena;
  program.capacity = 256;
  program.decls = arena_alloc(arena, program.capacity * sizeof(struct Decl *));
  program.length = 0;

  struct SymbolTable table = {0};
  table.scope = arena_alloc(arena, sizeof(struct Scope));
  table.scope->symbols = hashmap_new(arena, 128);
  table.exports = hashmap_new(arena, 8);
  table.scope->prev = NULL;
  table.scope->expectType = NULL;
  table.scope->retType = NULL;
  table.scope->currentStruct = NULL;
  table.program = &program;
  table.loop = 0;

  struct IRModule ir = {0};
  ir.globals_cap = 4;
  ir.globals = arena_alloc(arena, ir.globals_cap * sizeof(struct IRGlobal));
  ir.globals_len = 0;
  ir.init.blocks_cap = 1;
  ir.init.blocks = arena_alloc(arena, sizeof(struct IRBasicBlock));
  ir.init.blocks_len = 0;
  ir.functions_cap = 4;
  ir.functions = arena_alloc(arena, ir.functions_cap * sizeof(struct IRFunction));
  ir.functions_len = 0;

  lexer(path, code, &tokens, arena);
  parser(&tokens, &program);
  semantic(&table, modules);
  loweringAST(&program, &ir);
  showIR(&ir, arena);
  return table.exports;
}
