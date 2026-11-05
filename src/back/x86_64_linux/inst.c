#include "x86_64_linux.h"

void x86_64_linux_inst(struct LIRInstruction *inst, FILE *file, struct Arch *arch) {
  if (inst->opcode == LIR_COPY || inst->opcode == LIR_STORE) fprintf(file, "  mov");
  else if (inst->opcode == LIR_LOAD) fprintf(file, "  lea");
  else if (inst->opcode == LIR_ADD) fprintf(file, "  add");
  else if (inst->opcode == LIR_SUB) fprintf(file, "  sub");
  else if (inst->opcode == LIR_MUL) fprintf(file, "  imul");
  else if (inst->opcode == LIR_DIV) fprintf(file, "  %s", inst->is_unsigned ? "div" : "idiv");
  else if (inst->opcode == LIR_AND) fprintf(file, "  and");
  else if (inst->opcode == LIR_XOR) fprintf(file, "  xor");
  else if (inst->opcode == LIR_OR) fprintf(file, "  or");
  else if (inst->opcode == LIR_SHL) fprintf(file, "  shl");
  else if (inst->opcode == LIR_ASR) fprintf(file, "  sar");
  else if (inst->opcode == LIR_LSR) fprintf(file, "  shr");
  else if (inst->opcode == LIR_CMP) fprintf(file, "  cmp");
  else if (inst->opcode == LIR_FCMP) fprintf(file, "  comi");

  else if (inst->opcode == LIR_SETCC) {
    fprintf(file, "  set%s ", x86_64_linux_conds(inst->operands[0].imm));
    x86_64_linux_operand(&inst->operands[1], file);
    fprintf(file, "\n");
    return;
  }
  
  else if (inst->opcode == LIR_JCOND) {
    fprintf(file, "  j%s ", x86_64_linux_conds(inst->operands[0].imm));
    x86_64_linux_operand(&inst->operands[1], file);
    fprintf(file, "\n");
    return;
  }

  else if (inst->opcode == LIR_JMP) {
    fprintf(file, "  jmp ");
    x86_64_linux_operand(&inst->operands[0], file);
    fprintf(file, "\n");
    return;
  }

  else if (inst->opcode == LIR_TRUNC) {
    uint8_t is_float = inst->operands[inst->operands_len - 1].reg >= arch->iregisters_len;
    if (is_float)
      fprintf(file, "  cvt%s2", x86_64_linux_inst_size(inst->operands[0].size, 1));
    else fprintf(file, "  mov");
  }

  else if (inst->opcode == LIR_EXT) {
    uint8_t is_float = inst->operands[inst->operands_len - 1].reg >= arch->iregisters_len;
    if (is_float)
      fprintf(file, "  cvt%s2", x86_64_linux_inst_size(inst->operands[0].size, 1));
    else fprintf(file, "  mov%s%s", inst->is_unsigned ? "z" : "s", x86_64_linux_inst_size(inst->operands[0].size, 0));
  }

  else if (inst->opcode == LIR_CVT) {
    uint8_t org_is_float = inst->operands[0].reg >= arch->iregisters_len;
    uint8_t dst_is_float = inst->operands[inst->operands_len - 1].reg >= arch->iregisters_len;

    if (org_is_float)
      fprintf(file, "  cvtt%s2", x86_64_linux_inst_size(inst->operands[0].size, 1));
    else fprintf(file, "  cvtsi2");

    if (!dst_is_float)
      fprintf(file, "si");
  }

  else if (inst->opcode == LIR_RET) {
    fprintf(file, "  ret\n");
    return;
  }

  else if (inst->opcode == LIR_TARGET_SPECIFIC) {
    x86_64_linux_extension(inst->target_extension, inst->operands[inst->operands_len - 1].size, file);
    return;
  }

  uint8_t is_float = inst->operands[inst->operands_len - 1].reg >= arch->iregisters_len;
  fprintf(file, "%s ", x86_64_linux_inst_size(inst->operands[inst->operands_len - 1].size, is_float));
  for (int i = 0; i < inst->operands_len; i++) {
    if (i > 0) fprintf(file, ", ");
    x86_64_linux_operand(&inst->operands[i], file);
  }

  fprintf(file, "\n");
}
