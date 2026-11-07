#include "ir.h"
#include <stdio.h>

void printType(struct IRType *type) {
  if (type->kind == IRTYPEK_OTHER) {
#define X(name, str) \
    if (name == type->other) \
      printf("%s ", str);
    IRTYPES
#undef X
  }

  else if (type->kind == IRTYPEK_ARRAY) {
    printf("[ %ld x ", type->array.size);
    printType(type->array.type);
    printf("] ");
  }

  else if (type->kind == IRTYPEK_STRUCT) {
    printf("{ ");
    for (int i = 0; i < type->struct_.types_len; i++) {
      if (i > 0) printf(", ");
      printType(&type->struct_.types[i]);
    }
    printf("} ");
  }
}

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
  else if (value->kind == Value_Char)
    printf("%c", (char)value->u);
  else if (value->kind == Value_Float)
    printf("%ff", value->f);
  else if (value->kind == Value_Double)
    printf("%f", value->d);
  else if (value->kind == Value_String)
    printf("\"%.*s\\0\"", value->s->length, value->s->start);

  else if (value->kind == Value_Array) {
    printf("{ ");
    for (int i = 0; i < value->a.elems_len; i++) {
      if (i > 0) printf(", ");
      printValue(&value->a.elems[i]);
    }
    printf(" }");
  }
}

static void printOperand(struct IROperand *op);

static void printInst(struct IRInstruction *inst) { 
  if (inst->vl != -1)
    printf("%%%ld = ", inst->vl);

  enum IROpcode opcode = inst->opcode;
  printOpcode(opcode);

  for (int l = 0; l < inst->operands_len; l++) {
    if (l > 0) printf(", ");
    struct IROperand *op = &inst->operands[l];
    printOperand(op);
  }

  printf("\n    ");
}

static void printOperand(struct IROperand *op) {
  printType(&op->op_type);

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

  else if (op->kind == Operand_Type)
    printType(&op->type);

  else if (op->kind == Operand_Call) {
    printOperand(op->call.operand);
    printf("(");
    for (int i = 0; i < op->call.args_len; i++) {
      if (i > 0) printf(", ");
      printOperand(&op->call.args[i]);
    }
    printf(")");
  }

  else if (op->kind == Operand_String)
    printf("%.*s", op->string->length, op->string->start);
}

void showIR(struct IRModule *ir) {
  printf("MIR\n\n");
  for (int i = 0; i < ir->globals_len; i++) {
    printf("@%ld = ", ir->globals[i].vl);
    printOperand(&ir->globals[i].value);
    printf("\n");
  }

  for (int i = 0; i < ir->functions_len; i++) {
    printf("define %.*s(", ir->functions[i].name->length, ir->functions[i].name->start);
    for (int j = 0; j < ir->functions[i].params_len; j++) {
      if (j > 0) printf(", ");
      printType(&ir->functions[i].params[j].type);
      printf("%%%ld", ir->functions[i].params[j].vl);
    }

    printf(")\n  ");
    for (int j = 0; j < ir->functions[i].blocks_len; j++) {
      if (j == 0) printf("entry:\n    ");
      else printf("b%d:\n    ", j); 

      for (int k = 0; k < ir->functions[i].blocks[j].instructions_len; k++)
        printInst(&ir->functions[i].blocks[j].instructions[k]);

      printf("\n  ");
    }
    printf("\n");
  }
}
