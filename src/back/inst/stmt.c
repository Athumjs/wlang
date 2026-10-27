#include "inst.h"

void resolveRet_inst(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->operands_len == 1 && (inst->operands[0].kind != Operand_Register || regs[inst->operands[0].ref] != 0)) {
    fprintf(file, "  mov %s, ", reg_names[0][sizeOf(&inst->type)]);
    resolveOperand_code(live, &inst->operands[0], file);
    fprintf(file, "\n");
  };

  fprintf(file, "  ret\n");
}

int resolveStmt_code(int64_t *regs, struct Live *live, struct IRInstruction *inst, FILE *file) {
  if (inst->opcode == OPCODE_RET) resolveRet_inst(regs, live, inst, file);
  else return 0;
  return 1;
}
