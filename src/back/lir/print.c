#include "lir.h"
#include "../backs.h"

static void printOpcode(enum LIROpcodes opcode) {
#define X(name, str) \
  if (opcode == name) \
    printf("%s ", str);
  LIROPCODES
#undef X
}

static void printOperand(struct LIROperand *op) {
  if (op->kind == LIR_Immediate)
    printf("%ld", op->imm);
  else if (op->kind == LIR_Vreg)
    printf("Vr%ld", op->reg);
  else if (op->kind == LIR_Register)
    printf("R%ld", op->reg);
  else if (op->kind == LIR_Label || op->kind == LIR_Global)
    printf("%s", op->label);

  else if (op->kind == LIR_Float)
    printf("float_%ld(R%ld)", op->mem.disp, op->mem.base_reg);

  else if (op->kind == LIR_Address) {
    printf("%ld(%s%ld", op->mem.disp, op->mem.is_base_vreg ? "Vr" : "R", op->mem.base_reg);

    if (op->mem.index_reg != -1) {
      printf(", %s%ld", op->mem.is_index_vreg ? "Vr" : "R", op->mem.index_reg);

      if (op->mem.scale > 1)
        printf(", %d", op->mem.scale);
    }

    printf(")");
  }
  
  printf(":%d", op->size);
}

static void printInst(struct LIRInstruction *inst) {
  printf("  ");
  printOpcode(inst->opcode);

  if (inst->opcode == LIR_TARGET_SPECIFIC)
    printf("%ld", inst->target_extension);

  for (int i = 0; i < inst->operands_len; i++) {
    if (i > 0) printf(", ");
    printOperand(&inst->operands[i]);
  }

  printf("\n");
}

void showLIR(struct LIRFunction *funcs, size_t funcs_len) {
  printf("LIR\n\n");

  for (int i = 0; i < funcs_len; i++) {
    printf("define %.*s (stack_size: %ld)\n", funcs[i].name->length, funcs[i].name->start, funcs[i].stack_size);
    for (int j = 0; j < funcs[i].blocks_len; j++) {
      if (j > 0) printf(".Lb%d:\n", j);
      for (int k = 0; k < funcs[i].blocks[j].instructions_len; k++)
        printInst(&funcs[i].blocks[j].instructions[k]);
    }
  }
}
