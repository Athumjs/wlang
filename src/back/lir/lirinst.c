#include "lir.h"

void lirASM(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_ADD;

  if (irinst->opcode == OPCODE_SUB)
    opc = LIR_SUB;
  else if (irinst->opcode == OPCODE_MUL)
    opc = LIR_MUL;

  if (ctx->arch->is_3_operands) {
    struct LIRInstruction *inst = lir_newInst(opc, 3, block, ctx->arena, &irinst->type);
    liroperand(&inst->operands[0], &irinst->operands[1], ctx);
    liroperand(&inst->operands[1], &irinst->operands[0], ctx);
    lirop_reg(&inst->operands[2], &ctx->live->intervals[irinst->vl], ctx); 
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx); 

  struct LIRInstruction *op = lir_newInst(opc, 2, block, ctx->arena, &irinst->type);
  liroperand(&op->operands[0], &irinst->operands[1], ctx);
  lirop_reg(&op->operands[1], &ctx->live->intervals[irinst->vl], ctx);
}

void lirdiv(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct Interval *interval = &ctx->live->intervals[irinst->vl];

  if (ctx->arch->is_3_operands) {
    struct LIRInstruction *inst = lir_newInst(LIR_DIV, 3, block, ctx->arena, &irinst->type);
    liroperand(&inst->operands[0], &irinst->operands[1], ctx);
    liroperand(&inst->operands[1], &irinst->operands[0], ctx);
    lirop_reg(&inst->operands[2], interval, ctx);
    return;
  }

  struct Interval *interval_reg = NULL;
  for (int i = 0; i < ctx->live->length; i++) {
    if (ctx->live->intervals[i].reg != ctx->arch->div_register) continue;
    if (ctx->pos < ctx->live->intervals[i].start || ctx->pos > ctx->live->intervals[i].end) continue;
    interval_reg = &ctx->live->intervals[i];
  }

  if (irinst->operands[0].kind != Operand_Register || interval_reg->vreg != irinst->operands[0].ref) {
    if (interval_reg != NULL) {
      struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
      inst->operands[0].kind = LIR_Register;
      inst->operands[0].reg = ctx->arch->div_register;
      inst->operands[1].kind = LIR_Address;
      inst->operands[1].mem.base_reg = ctx->arch->stack_register;
      interval_reg->reg = -1;
      interval_reg->spillSlot = *ctx->stack_size;
      *ctx->stack_size += (inst->operands[1].size + 15) & ~15;
      inst->operands[1].mem.disp = interval_reg->spillSlot;
    }

    struct LIRInstruction *op = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    liroperand(&op->operands[0], &irinst->operands[0], ctx);
    op->operands[1].kind = LIR_Register;
    op->operands[1].reg = ctx->arch->div_register;
  }

  interval_reg = NULL;
  for (int i = 0; i < ctx->live->length; i++) {
    if (ctx->live->intervals[i].reg != ctx->arch->rem_register) continue;
    if (ctx->pos < ctx->live->intervals[i].start || ctx->pos > ctx->live->intervals[i].end) continue;
    interval_reg = &ctx->live->intervals[i];
  }

  if (interval_reg != NULL) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    inst->operands[0].kind = LIR_Register;
    inst->operands[0].reg = ctx->arch->rem_register;
    inst->operands[1].kind = LIR_Address;
    inst->operands[1].mem.base_reg = ctx->arch->stack_register;
    interval_reg->reg = -1;
    interval_reg->spillSlot = *ctx->stack_size;
    *ctx->stack_size += (inst->operands[1].size + 15) & ~15;
    inst->operands[1].mem.disp = interval_reg->spillSlot;
  }

  interval->reg = ctx->arch->div_register;
  struct LIRInstruction *ext = lir_newInst(LIR_TARGET_SPECIFIC, 0, block, ctx->arena, &irinst->type);
  ext->target_extension = 0;

  if (irinst->operands[1].kind != Operand_Register) {
    struct LIRInstruction *vl = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    liroperand(&vl->operands[0], &irinst->operands[1], ctx);
    vl->operands[1].kind = LIR_Address;
    vl->operands[1].mem.base_reg = ctx->arch->stack_register;
    *ctx->stack_size += (vl->operands[1].size + 15) & ~15;
    vl->operands[1].mem.disp = *ctx->stack_size;
    struct LIRInstruction *inst = lir_newInst(LIR_DIV, 1, block, ctx->arena, &irinst->type); 
    if (irinst->opcode == OPCODE_UDIV) inst->is_unsigned = 1;
    inst->operands[0].kind = LIR_Address;
    inst->operands[0].mem.base_reg = ctx->arch->stack_register;
    inst->operands[0].mem.disp = *ctx->stack_size;
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_DIV, 1, block, ctx->arena, &irinst->type); 
  if (irinst->opcode == OPCODE_UDIV) inst->is_unsigned = 1;
  liroperand(&inst->operands[0], &irinst->operands[1], ctx);
}

void lirconst(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
}

void lirret(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  if (irinst->operands_len == 1 && (irinst->operands[0].kind != Operand_Register || ctx->live->intervals[irinst->operands[0].ref].reg != ctx->arch->ret_register)) {
    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    liroperand(&inst->operands[0], &irinst->operands[0], ctx);
    inst->operands[1].kind = LIR_Register;
    inst->operands[1].reg = ctx->arch->ret_register;
  }

  lir_newInst(LIR_RET, 0, block, ctx->arena, &irinst->type);
}

void lirinst(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  if (irinst->opcode == OPCODE_CONST) lirconst(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_ADD || irinst->opcode == OPCODE_SUB || irinst->opcode == OPCODE_MUL) lirASM(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_FADD || irinst->opcode == OPCODE_FSUB || irinst->opcode == OPCODE_FMUL) lirASM(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_SDIV || irinst->opcode == OPCODE_UDIV || irinst->opcode == OPCODE_FDIV) lirdiv(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_RET) lirret(block, irinst, ctx);
}
