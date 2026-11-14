#include "lir.h"

void lir_lc(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);

  if (irinst->opcode == OPCODE_LOAD)
    lir_addOperandAddress(3, irinst->operands[0].ref, 1, -1, 0, 0, 0, &irinst->operands[0].op_type, inst, ctx);
  else lir_operand(inst, &irinst->operands[0], ctx);

  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, is_float(&irinst->operands[0].op_type), inst, ctx);
}

void lir_store(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  int64_t vl = -1; 
  uint8_t isFloat = 0;
  if (irinst->operands[0].kind == Operand_Global || is_float(&irinst->operands[0].op_type)) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_operand(inst, &irinst->operands[0], ctx);
    if (irinst->operands[0].kind != Operand_Global) isFloat = 1;
    lir_addOperandReg(1, -1, &irinst->operands[0].op_type, isFloat, inst, ctx);
    vl = inst->operands[1].reg;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  if (vl != -1)
    lir_addOperandReg(1, vl, &irinst->operands[0].op_type, isFloat, inst, ctx);
  else lir_operand(inst, &irinst->operands[0], ctx);
  if (irinst->operands[1].kind == Operand_Global)
    lir_operand(inst, &irinst->operands[1], ctx);
  else lir_addOperandAddress(3, irinst->operands[1].ref, 1, -1, 0, 0, 0, &irinst->operands[0].op_type, inst, ctx);
}

void lir_asm(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_ADD;

  if (irinst->opcode == OPCODE_SUB || irinst->opcode == OPCODE_FSUB)
    opc = LIR_SUB;
  else if (irinst->opcode == OPCODE_MUL || irinst->opcode == OPCODE_FMUL)
    opc = LIR_MUL;
  else if (irinst->opcode == OPCODE_FDIV)
    opc = LIR_DIV;

  uint8_t is_float = 0;
  if (irinst->opcode == OPCODE_FADD || irinst->opcode == OPCODE_FSUB || irinst->opcode == OPCODE_FMUL || irinst->opcode == OPCODE_FDIV)
    is_float = 1;

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, is_float, inst, ctx);

  struct LIRInstruction *op = lir_newInst(opc, 2, block, ctx->arena);
  lir_operand(op, &irinst->operands[1], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, is_float, op, ctx);
}

void lir_div(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_addOperandReg(0, ctx->arch->div_register, &irinst->operands[0].op_type, 0, inst, ctx);

  if (irinst->opcode != OPCODE_UDIV && irinst->opcode != OPCODE_UREM) {
 #if defined(__x86_64__) || defined(_M_X64)
  // EAX:EDX x86
  struct LIRInstruction *ext = lir_newInst(LIR_TARGET_SPECIFIC, 1, block, ctx->arena);
  ext->operands[0].size = sizeOf(&irinst->operands[0].op_type);
  ext->target_extension = 0;
#endif
  }

  if (irinst->operands[1].kind != Operand_Register) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_operand(inst, &irinst->operands[1], ctx);
    lir_addOperandReg(1, -1, &irinst->operands[0].op_type, 0, inst, ctx);
    struct LIRInstruction *div = lir_newInst(LIR_DIV, 1, block, ctx->arena);
    if (irinst->opcode == OPCODE_UDIV  || irinst->opcode == OPCODE_UREM) div->is_unsigned = 1;
    lir_addOperandReg(1, inst->operands[1].reg, &irinst->operands[0].op_type, 0, div, ctx);
  } else {
    struct LIRInstruction *div = lir_newInst(LIR_DIV, 1, block, ctx->arena);
    if (irinst->opcode == OPCODE_UDIV  || irinst->opcode == OPCODE_UREM) div->is_unsigned = 1;
    lir_operand(div, &irinst->operands[1], ctx);
  }

  struct LIRInstruction *op = lir_newInst(LIR_COPY, 2, block, ctx->arena);

  if (irinst->opcode == OPCODE_SREM || irinst->opcode == OPCODE_UREM)
    lir_addOperandReg(0, ctx->arch->rem_register, &irinst->operands[0].op_type, 0, op, ctx);
  else lir_addOperandReg(0, ctx->arch->div_register, &irinst->operands[0].op_type, 0, op, ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, op, ctx);
}

void lir_cast(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_CVT;

  if (irinst->opcode == OPCODE_TRUNC || irinst->opcode == OPCODE_FPTRUNC ||
      irinst->opcode == OPCODE_PTR2INT)
    opc = LIR_TRUNC;
  else if (irinst->opcode == OPCODE_SEXT || irinst->opcode == OPCODE_ZEXT ||
      irinst->opcode == OPCODE_SI2PTR || irinst->opcode == OPCODE_UI2PTR)
    opc = LIR_EXT;

  uint8_t is_float = 0;
  if (irinst->opcode == OPCODE_FPTRUNC || irinst->opcode == OPCODE_FPEXT || irinst->opcode == OPCODE_SI2FP || irinst->opcode == OPCODE_UI2FP)
    is_float = 1;

  struct LIRInstruction *inst = lir_newInst(opc, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[1].type, is_float, inst, ctx);
  if (opc == LIR_TRUNC) inst->operands[0].size = inst->operands[1].size;
  if (irinst->opcode == OPCODE_ZEXT || irinst->opcode == OPCODE_UI2PTR) inst->is_unsigned = 1;
}

void lir_axo(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_AND;

  if (irinst->opcode == OPCODE_BIT_XOR)
    opc = LIR_XOR;
  else if (irinst->opcode == OPCODE_BIT_OR)
    opc = LIR_OR;

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, inst, ctx);
  struct LIRInstruction *op = lir_newInst(opc, 2, block, ctx->arena);
  lir_operand(op, &irinst->operands[1], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, op, ctx);
}

void lir_sal(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_SHL;

  if (irinst->opcode == OPCODE_BIT_ASHR)
    opc = LIR_ASR;
  else if (irinst->opcode == OPCODE_BIT_LSHR)
    opc = LIR_LSR;

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, inst, ctx);

  if (irinst->operands[1].kind != Operand_Register) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_operand(inst, &irinst->operands[1], ctx);
    lir_addOperandReg(0, ctx->arch->shift_register, &irinst->operands[0].op_type, 0, inst, ctx);
    inst->operands[1].size = 1;
  }

  struct LIRInstruction *shift = lir_newInst(opc, 2, block, ctx->arena);
  if (irinst->operands[1].kind != Operand_Register)
    lir_addOperandReg(0, ctx->arch->shift_register, &irinst->operands[0].op_type, 0, shift, ctx);
  else lir_operand(shift, &irinst->operands[1], ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, shift, ctx);
}

void lir_alloca(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  ctx->stack_len += sizeOf(&irinst->operands[0].type);
  struct LIRInstruction *inst = lir_newInst(LIR_LOAD, 2, block, ctx->arena);
  lir_addOperandAddress(3, ctx->arch->stack_base_register, 0, -1, 0, -ctx->stack_len, 0, &irinst->operands[0].op_type, inst, ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, inst, ctx);
}

void lir_cmp(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(irinst->opcode == OPCODE_FCMP ? LIR_FCMP : LIR_CMP, 2, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  lir_operand(inst, &irinst->operands[1], ctx);

  struct FindInst find = findInst(ctx->func, irinst->vl);
  struct IRBasicBlock *b = ctx->func->blocks[find.block_index];

  if (b->instructions[find.inst_index + 1].opcode != OPCODE_BR) {
    struct LIRInstruction *inst = lir_newInst(LIR_SETCC, 2, block, ctx->arena);
    enum LIRCmp cond = LIRCMP_Ne;
    switch (irinst->operands[0].cmp.icmp) {
      case ICMP_Eq:
        cond = LIRCMP_Eq;
        break;
      case ICMP_Ne:
        cond = LIRCMP_Ne;
        break;
      case ICMP_SGt:
        cond = LIRCMP_SGt;
        break;
      case ICMP_SGe:
        cond = LIRCMP_SGe;
        break;
      case ICMP_SLt:
        cond = LIRCMP_SLt;
        break;
      case ICMP_SLe:
        cond = LIRCMP_SLe;
        break;
      case ICMP_UGt:
        cond = LIRCMP_UGt;
        break;
      case ICMP_UGe:
        cond = LIRCMP_UGe;
        break;
      case ICMP_ULt:
        cond = LIRCMP_ULt;
        break;
      case ICMP_ULe:
        cond = LIRCMP_ULe;
        break;
    }
    lir_addOperandConst(cond, &irinst->operands[0].op_type, inst);
    lir_addOperandReg(1, irinst->vl, &irinst->operands[0].op_type, 0, inst, ctx);
  }
}

void lir_br(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct IRInstruction *ircond = getInst(ctx->func, irinst->operands[0].ref);
  enum LIRCmp cond = LIRCMP_Eq;
  struct LIRInstruction *inst = lir_newInst(LIR_JCOND, 2, block, ctx->arena);
  block->sucs = arena_alloc(ctx->arena, 2 * sizeof(int64_t));
  block->sucs_len = 2;
  block->sucs[0] = irinst->operands[1].ref;
  block->sucs[1] = irinst->operands[2].ref;
  if (ircond->opcode == OPCODE_ICMP) {
    switch (ircond->operands[0].cmp.icmp) {
      case ICMP_Eq:
        cond = LIRCMP_Eq;
        break;
      case ICMP_Ne:
        cond = LIRCMP_Ne;
        break;
      case ICMP_SGt:
        cond = LIRCMP_SGt;
        break;
      case ICMP_SGe:
        cond = LIRCMP_SGe;
        break;
      case ICMP_SLt:
        cond = LIRCMP_SLt;
        break;
      case ICMP_SLe:
        cond = LIRCMP_SLe;
        break;
      case ICMP_UGt:
        cond = LIRCMP_UGt;
        break;
      case ICMP_UGe:
        cond = LIRCMP_UGe;
        break;
      case ICMP_ULt:
        cond = LIRCMP_ULt;
        break;
      case ICMP_ULe:
        cond = LIRCMP_ULe;
        break;
    }
  } else {
    switch (ircond->operands[0].cmp.fcmp) {
      case FCMP_OEq:
        cond = LIRCMP_Eq;
        break;
      case FCMP_ONe:
        cond = LIRCMP_Ne;
        break;
      case FCMP_OGt:
        cond = LIRCMP_UGt;
        break;
      case FCMP_OGe:
        cond = LIRCMP_UGe;
        break;
      case FCMP_OLt:
        cond = LIRCMP_ULt;
        break;
      case FCMP_OLe:
        cond = LIRCMP_ULe;
        break;
    }
  }

  lir_addOperandConst(cond, &irinst->operands[0].op_type, inst);
  lir_operand(inst, &irinst->operands[1], ctx);
  struct LIRInstruction *jmp = lir_newInst(LIR_JMP, 1, block, ctx->arena);
  lir_operand(jmp, &irinst->operands[2], ctx);
}

void lir_jmp(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_JMP, 1, block, ctx->arena);
  lir_operand(inst, &irinst->operands[0], ctx);
  block->sucs = arena_alloc(ctx->arena, sizeof(int64_t));
  block->sucs[0] = irinst->operands[0].ref;
  block->sucs_len = 1;
}

void lir_call(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  size_t floats_args = 0;
  size_t ints_args = 0;
  for (int i = 0; i < irinst->operands[0].call.args_len; i++) {
    struct IROperand *operand = &irinst->operands[0].call.args[i];
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_operand(inst, operand, ctx);
    if (operand->op_type.kind == IRTYPEK_OTHER && (operand->op_type.other == IRTYPE_f32 || operand->op_type.other == IRTYPE_f64)) {
      floats_args++;
      if ((floats_args - 1) >= ctx->arch->arg_fregisters_len) {
        ctx->stack_len += ((floats_args - 1) - ctx->arch->arg_fregisters_len) * 8;
        lir_addOperandAddress(3, ctx->arch->stack_top_register, 0, -1, 0, -ctx->stack_len, 0, &operand->op_type, inst, ctx);
        continue;
      }
      lir_addOperandReg(0, ctx->arch->arg_registers[(floats_args - 1) + ctx->arch->arg_iregisters_len], &operand->op_type, 1, inst, ctx);
      continue;
    }
    ints_args++;
    if ((ints_args - 1) >= ctx->arch->arg_iregisters_len) {
      ctx->stack_len += ((ints_args - 1) - ctx->arch->arg_iregisters_len) * 8;
      lir_addOperandAddress(3, ctx->arch->stack_top_register, 0, -1, 0, -ctx->stack_len, 0, &operand->op_type, inst, ctx);
      continue;
    }
    lir_addOperandReg(0, ctx->arch->arg_registers[ints_args - 1], &operand->op_type, 0, inst, ctx);
  }
  if (irinst->operands[0].call.is_extern) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_addOperandConst(floats_args, &irinst->operands[0].op_type, inst);
    lir_addOperandReg(0, ctx->arch->ret_iregister, &irinst->operands[0].op_type, 0, inst, ctx);
    inst->operands[0].size = 4;
    inst->operands[1].size = 4;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_CALL, 1, block, ctx->arena);
  lir_operand(inst, irinst->operands[0].call.operand, ctx);
  ctx->has_call = 1;
  struct LIRInstruction *op = lir_newInst(LIR_COPY, 2, block, ctx->arena);
  if (irinst->operands[0].call.operand->op_type.kind == IRTYPEK_OTHER &&
      (irinst->operands[0].call.operand->op_type.other == IRTYPE_f32 || irinst->operands[0].call.operand->op_type.other == IRTYPE_f64))
    lir_addOperandReg(0, ctx->arch->ret_fregister, &irinst->operands[0].call.operand->op_type, 1, op, ctx);
  else lir_addOperandReg(0, ctx->arch->ret_iregister, &irinst->operands[0].call.operand->op_type, 0, op, ctx);
  lir_addOperandReg(1, irinst->vl, &irinst->operands[0].call.operand->op_type, is_float(&irinst->operands[0].call.operand->op_type), op, ctx);
}

void lir_ret(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  if (irinst->operands_len == 1) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena);
    lir_operand(inst, &irinst->operands[0], ctx);
    if (irinst->operands[0].op_type.kind == IRTYPEK_OTHER && 
        (irinst->operands[0].op_type.other == IRTYPE_f32 || irinst->operands[0].op_type.other == IRTYPE_f64))
      lir_addOperandReg(0, ctx->arch->ret_fregister, &irinst->operands[0].op_type, 1, inst, ctx);
    else lir_addOperandReg(0, ctx->arch->ret_iregister, &irinst->operands[0].op_type, 0, inst, ctx);
  }

  lir_newInst(LIR_RET, 0, block, ctx->arena);
}

void lir_inst(struct LIRBasicBlocks *block, struct IRInstruction *inst, struct LIRContext *ctx) {
  if (inst->opcode == OPCODE_CONST || inst->opcode == OPCODE_LOAD) lir_lc(block, inst, ctx);
  else if (inst->opcode == OPCODE_STORE) lir_store(block, inst, ctx);
  else if (inst->opcode == OPCODE_ADD || inst->opcode == OPCODE_SUB || inst->opcode == OPCODE_MUL
      || inst->opcode == OPCODE_FADD || inst->opcode == OPCODE_FSUB || inst->opcode == OPCODE_FMUL ||
      inst->opcode == OPCODE_FDIV) lir_asm(block, inst, ctx);
  else if (inst->opcode == OPCODE_SDIV || inst->opcode == OPCODE_UDIV
      || inst->opcode == OPCODE_SREM || inst->opcode == OPCODE_UREM) lir_div(block, inst, ctx);
  else if (inst->opcode == OPCODE_TRUNC || inst->opcode == OPCODE_FPTRUNC ||
      inst->opcode == OPCODE_FP2SI || inst->opcode == OPCODE_FP2UI ||
      inst->opcode == OPCODE_SI2FP || inst->opcode == OPCODE_UI2FP ||
      inst->opcode == OPCODE_SEXT || inst->opcode == OPCODE_ZEXT ||
      inst->opcode == OPCODE_PTR2INT || inst->opcode == OPCODE_UI2PTR ||
      inst->opcode == OPCODE_SI2PTR || inst->opcode == OPCODE_FPEXT) lir_cast(block, inst, ctx);
  else if (inst->opcode == OPCODE_BIT_AND || inst->opcode == OPCODE_BIT_XOR || inst->opcode == OPCODE_BIT_OR) lir_axo(block, inst, ctx);
  else if (inst->opcode == OPCODE_BIT_SHL || inst->opcode == OPCODE_BIT_ASHR || inst->opcode == OPCODE_BIT_LSHR) lir_sal(block, inst, ctx);
  else if (inst->opcode == OPCODE_ALLOCA) lir_alloca(block, inst, ctx);
  else if (inst->opcode == OPCODE_ICMP || inst->opcode == OPCODE_FCMP) lir_cmp(block, inst, ctx);
  else if (inst->opcode == OPCODE_BR) lir_br(block, inst, ctx);
  else if (inst->opcode == OPCODE_JMP) lir_jmp(block, inst, ctx);
  else if (inst->opcode == OPCODE_CALL) lir_call(block, inst, ctx);
  else if (inst->opcode == OPCODE_RET) lir_ret(block, inst, ctx);
}
