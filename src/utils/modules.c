#include <utils/modules.h>
#include <front/semantic.h>
#include <middle/irgen.h>
#include <utils/error.h>
#include <stdio.h>

void printOpcode(enum IROpcode opcode) {
  if (opcode == Opcode_Ret) printf("ret ");
  else if (opcode == Opcode_Add) printf("add ");
  else if (opcode == Opcode_FAdd) printf("fadd ");
  else if (opcode == Opcode_Sub) printf("sub ");
  else if (opcode == Opcode_FSub) printf("fsub ");
  else if (opcode == Opcode_Mul) printf("mul ");
  else if (opcode == Opcode_FMul) printf("fmul ");
  else if (opcode == Opcode_SDiv) printf("sdiv ");
  else if (opcode == Opcode_UDiv) printf("udiv ");
  else if (opcode == Opcode_FDiv) printf("fdiv ");
  else if (opcode == Opcode_SRem) printf("srem ");
  else if (opcode == Opcode_URem) printf("urem ");
  else if (opcode == Opcode_FRem) printf("frem ");
  else if (opcode == Opcode_Bit_And) printf("btand ");
  else if (opcode == Opcode_Bit_Or) printf("btor ");
  else if (opcode == Opcode_Bit_Xor) printf("xor ");
  else if (opcode == Opcode_Bit_Shl) printf("shl ");
  else if (opcode == Opcode_Bit_AShr) printf("ashr ");
  else if (opcode == Opcode_Bit_LShr) printf("lshr ");
  else if (opcode == Opcode_Br) printf("br ");
  else if (opcode == Opcode_Jmp) printf("jmp ");
  else if (opcode == Opcode_Alloca) printf("alloca ");
  else if (opcode == Opcode_Store) printf("store ");
  else if (opcode == Opcode_Load) printf("load ");
}

void printValue(struct Value *value) {
  if (value->kind == Value_Int)
    printf("%ld", value->i);
  else if (value->kind == Value_UInt)
    printf("%ldu", value->u);
  else if (value->kind == Value_Float)
    printf("%ff", value->f);
  else if (value->kind == Value_Double)
    printf("%f", value->d);
}

void printOperand(struct IROperand *op, struct Arena *arena);

void printInst(struct IRInstruction *inst, struct Arena *arena) { 
  if (inst->result != -1)
    printf("%%%ld = ", inst->result);

  enum IROpcode opcode = inst->opcode;
  printOpcode(opcode);

  for (int l = 0; l < inst->operands_len; l++) {
    if (l > 0) printf(", ");
    struct IROperand *op = &inst->operands[l];
    printOperand(op, arena);
  }

  printf("\n    ");
}

void printOperand(struct IROperand *op, struct Arena *arena) {
  if (op->kind == Operand_Register)
    printf("%%%ld", op->reg);
  else if (op->kind == Operand_Pointer) {
    printf("ptr ");
    if (op->pointer.kind == Pointer_Local)
      printf("%%%ld", op->pointer.inst->result);
    else printf("@%ld", op->pointer.global->result);
  } else if (op->kind == Operand_Block)
    printf("b%ld", op->block);
  else if (op->kind == Operand_Constant)
    printValue(&op->constant);
  else if (op->kind == Operand_Type) {
    struct String type = getType(op->type, arena);
    printf("%.*s", type.length, type.start);
  }
}

void showIR(struct IRModule *ir, struct Arena *arena) {
  printf("IR\n\n");
  for (int i = 0; i < ir->globals_len; i++) {
    struct String type = getType(ir->globals[i].type, arena);
    printf("@%ld = global %.*s ", ir->globals[i].result, type.length, type.start);
    printOperand(&ir->globals[i].value, arena);
    printf("\n");
  }

  if (ir->init.blocks[0].instructions_len > 0) printf("\n#init:\n  ");
  for (int i = 0; i < ir->init.blocks[0].instructions_len; i++) {
    if (ir->init.blocks[0].instructions[i].result != -1)
      printf("%%%ld = ", ir->init.blocks[0].instructions[i].result);

    enum IROpcode opcode = ir->init.blocks[0].instructions[i].opcode; 
    printOpcode(opcode);

    for (int l = 0; l < ir->init.blocks[0].instructions[i].operands_len; l++) {
      if (l > 0) printf(", ");
      struct IROperand *op = &ir->init.blocks[0].instructions[i].operands[l];
      printOperand(op, arena);
    }

    printf("\n  ");
  }

  printf("\n");
  for (int i = 0; i < ir->functions_len; i++) {
    printf("fn %.*s:\n  ", ir->functions[i].name->length, ir->functions[i].name->start);
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

  struct Tokens tokens;
  tokens.capacity = 256;
  tokens.token = arena_alloc(arena, tokens.capacity * sizeof(struct Token));
  tokens.length = 0;

  struct Program program;
  program.filename = path;
  program.arena = arena;
  program.capacity = 256;
  program.decls = arena_alloc(arena, program.capacity * sizeof(struct Decl *));
  program.length = 0;

  struct SymbolTable table;
  table.scope = arena_alloc(arena, sizeof(struct Scope));
  table.scope->symbols = hashmap_new(arena, 128);
  table.exports = hashmap_new(arena, 8);
  table.scope->prev = NULL;
  table.scope->expectType = NULL;
  table.scope->retType = NULL;
  table.scope->currentStruct = NULL;
  table.scope->onLoop = 0;
  table.program = &program;

  struct IRModule ir;
  ir.globals_cap = 4;
  ir.globals = arena_alloc(arena, ir.globals_cap * sizeof(struct IRGlobal));
  ir.globals_len = 0;
  ir.init.blocks_cap = 1;
  ir.init.blocks = arena_alloc(arena, sizeof(struct IRFunction));
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
