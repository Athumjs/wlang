#include "lir.h"
#include "../backs.h"

void lirCL(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);

  if (irinst->opcode == OPCODE_LOAD) {
    int64_t reg = inst->operands[0].reg;
    inst->operands[0].kind = LIR_Address;
    inst->operands[0].mem.base_reg = reg;
    inst->operands[0].mem.index_reg = -1;
  }

  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
}

void lirstore(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  if (irinst->operands[0].kind == Operand_Register && ctx->live->intervals[irinst->operands[0].ref].reg == -1) {
    struct LIRInstruction *inst = lir_newInst(LIR_LOAD, 2, block, ctx->arena, &irinst->type);
    liroperand(&inst->operands[0], &irinst->operands[0], ctx);
    lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
    struct LIRInstruction *op = lir_newInst(LIR_STORE, 2, block, ctx->arena, &irinst->type);
    lirop_reg(&op->operands[0], &ctx->live->intervals[irinst->vl], ctx);
    liroperand(&op->operands[1], &irinst->operands[1], ctx);
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_STORE, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  liroperand(&inst->operands[1], &irinst->operands[1], ctx);
}

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
      *ctx->stack_size += inst->operands[1].size;
      interval_reg->spillSlot = *ctx->stack_size;
      inst->operands[1].mem.disp = -interval_reg->spillSlot;
      inst->operands[1].mem.index_reg = -1;
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
    *ctx->stack_size += inst->operands[1].size;
    interval_reg->spillSlot = *ctx->stack_size;
    inst->operands[1].mem.disp = -interval_reg->spillSlot;
    inst->operands[1].mem.index_reg = -1;
  }

  interval->reg = (irinst->opcode == OPCODE_SREM || irinst->opcode == OPCODE_UREM) ? ctx->arch->rem_register : ctx->arch->div_register;
  struct LIRInstruction *ext = lir_newInst(LIR_TARGET_SPECIFIC, 0, block, ctx->arena, &irinst->type);
  ext->target_extension = 0;

  if (irinst->operands[1].kind != Operand_Register) {
    struct LIRInstruction *vl = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    liroperand(&vl->operands[0], &irinst->operands[1], ctx);
    vl->operands[1].kind = LIR_Address;
    vl->operands[1].mem.base_reg = ctx->arch->stack_register;
    *ctx->stack_size += vl->operands[1].size;
    vl->operands[1].mem.disp = -(*ctx->stack_size);
    vl->operands[1].mem.index_reg = -1;
    struct LIRInstruction *inst = lir_newInst(LIR_DIV, 1, block, ctx->arena, &irinst->type); 
    if (irinst->opcode == OPCODE_UDIV || irinst->opcode == OPCODE_UREM) inst->is_unsigned = 1;
    inst->operands[0].kind = LIR_Address;
    inst->operands[0].mem.base_reg = ctx->arch->stack_register;
    inst->operands[0].mem.disp = -(*ctx->stack_size);
    inst->operands[0].mem.index_reg = -1;
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_DIV, 1, block, ctx->arena, &irinst->type); 
  if (irinst->opcode == OPCODE_UDIV || irinst->opcode == OPCODE_UREM) inst->is_unsigned = 1;
  liroperand(&inst->operands[0], &irinst->operands[1], ctx);
}

void lirTC(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_CVT;

  if (irinst->opcode == OPCODE_TRUNC || irinst->opcode == OPCODE_FPTRUNC)
    opc = LIR_TRUNC;

  struct LIRInstruction *inst = lir_newInst(opc, 2, block, ctx->arena, &irinst->operands[1].type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  if (irinst->opcode == OPCODE_FPTRUNC || irinst->opcode == OPCODE_FP2SI || irinst->opcode == OPCODE_FP2UI)
    inst->operands[0].size = sizeOf(&irinst->type);
  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
}

void lirext(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  struct LIRInstruction *inst = lir_newInst(LIR_EXT, 2, block, ctx->arena, &irinst->operands[1].type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  inst->operands[0].size = sizeOf(&irinst->type);
  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
  if (irinst->opcode == OPCODE_ZEXT) inst->is_unsigned = 1;
}

void lirAXO(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_AND;

  if (irinst->opcode == OPCODE_BIT_XOR)
    opc = LIR_XOR;
  else if (irinst->opcode == OPCODE_BIT_OR)
    opc = LIR_OR;

  if (ctx->arch->is_3_operands) {
    struct LIRInstruction *op = lir_newInst(opc, 3, block, ctx->arena, &irinst->type);
    liroperand(&op->operands[0], &irinst->operands[1], ctx);
    liroperand(&op->operands[0], &irinst->operands[0], ctx);
    lirop_reg(&op->operands[1], &ctx->live->intervals[irinst->vl], ctx);
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);
  lirop_reg(&inst->operands[1], &ctx->live->intervals[irinst->vl], ctx);
  struct LIRInstruction *op = lir_newInst(opc, 2, block, ctx->arena, &irinst->type);
  liroperand(&op->operands[0], &irinst->operands[1], ctx);
  lirop_reg(&op->operands[1], &ctx->live->intervals[irinst->vl], ctx);
}

void lirSAL(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  enum LIROpcodes opc = LIR_SHL;

  if (irinst->opcode == OPCODE_BIT_ASHR)
    opc = LIR_ASR;
  else if (irinst->opcode == OPCODE_BIT_LSHR)
    opc = LIR_LSR;

  if (ctx->arch->is_3_operands) {
    struct LIRInstruction *op = lir_newInst(opc, 3, block, ctx->arena, &irinst->type);
    liroperand(&op->operands[0], &irinst->operands[1], ctx);
    liroperand(&op->operands[1], &irinst->operands[0], ctx);
    lirop_reg(&op->operands[2], &ctx->live->intervals[irinst->vl], ctx);
    return;
  }

  struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
  liroperand(&inst->operands[0], &irinst->operands[0], ctx);

  struct Interval *interval = &ctx->live->intervals[irinst->vl];

  if (irinst->operands[1].kind == Operand_Register && interval->reg == ctx->arch->shift_register) {
    interval->reg = -1;

    if (ctx->live->intervals[irinst->operands[1].ref].reg != ctx->arch->shift_register) {
      struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
      liroperand(&inst->operands[0], &irinst->operands[1], ctx);
      inst->operands[0].size = 1;
      inst->operands[1].kind = LIR_Register;
      inst->operands[1].reg = ctx->arch->shift_register;
      inst->operands[1].size = 1;
      ctx->live->intervals[irinst->operands[1].ref].reg = ctx->arch->shift_register;
    }
  } else if (irinst->operands[1].kind == Operand_Register) {
    struct Interval *interval_reg = NULL;
    for (int i = 0; i < ctx->live->length; i++) {
      if (ctx->live->intervals[i].reg != ctx->arch->shift_register) continue;
      if (ctx->pos < ctx->live->intervals[i].start || ctx->pos > ctx->live->intervals[i].end) continue;
      interval_reg = &ctx->live->intervals[i];
    }

    if (interval_reg != NULL) {
      struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
      inst->operands[0].kind = LIR_Register;
      inst->operands[0].reg = ctx->arch->shift_register;
      inst->operands[1].kind = LIR_Address;
      inst->operands[1].mem.base_reg = ctx->arch->stack_register;
      interval_reg->reg = -1;
      *ctx->stack_size += inst->operands[1].size;
      interval_reg->spillSlot = *ctx->stack_size;
      inst->operands[1].mem.disp = -interval_reg->spillSlot;
      inst->operands[1].mem.index_reg = -1;
    }

    struct LIRInstruction *inst = lir_newInst(LIR_COPY, 2, block, ctx->arena, &irinst->type);
    liroperand(&inst->operands[0], &irinst->operands[1], ctx);
    inst->operands[0].size = 1;
    inst->operands[1].kind = LIR_Register;
    inst->operands[1].reg = ctx->arch->shift_register;
    inst->operands[1].size = 1;
    ctx->live->intervals[irinst->operands[1].ref].reg = ctx->arch->shift_register;
    ctx->live->intervals[irinst->operands[1].ref].reg = ctx->arch->shift_register;
  }

  lirop_reg(&inst->operands[1], interval, ctx);
  struct LIRInstruction *op = lir_newInst(opc, 2, block, ctx->arena, &irinst->type);
  liroperand(&op->operands[0], &irinst->operands[1], ctx);
  if (irinst->operands[1].kind == Operand_Register) op->operands[0].size = 1;
  op->operands[1] = inst->operands[1];
}

void liralloca(struct LIRBasicBlocks *block, struct IRInstruction *irinst, struct LIRContext *ctx) {
  *ctx->stack_size += sizeOf(&irinst->type);
  ctx->live->intervals[irinst->vl].spillSlot = *ctx->stack_size;
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
  if (irinst->opcode == OPCODE_CONST || irinst->opcode == OPCODE_LOAD) lirCL(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_STORE) lirstore(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_ADD || irinst->opcode == OPCODE_SUB || irinst->opcode == OPCODE_MUL) lirASM(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_FADD || irinst->opcode == OPCODE_FSUB || irinst->opcode == OPCODE_FMUL) lirASM(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_SDIV || irinst->opcode == OPCODE_UDIV || irinst->opcode == OPCODE_FDIV || 
      irinst->opcode == OPCODE_SREM || irinst->opcode == OPCODE_UREM) lirdiv(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_TRUNC || irinst->opcode == OPCODE_FPTRUNC ||
      irinst->opcode == OPCODE_FP2SI || irinst->opcode == OPCODE_FP2UI ||
      irinst->opcode == OPCODE_SI2FP || irinst->opcode == OPCODE_UI2FP) lirTC(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_SEXT || irinst->opcode == OPCODE_ZEXT || irinst->opcode == OPCODE_FPEXT) lirext(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_BIT_AND || irinst->opcode == OPCODE_BIT_XOR || irinst->opcode == OPCODE_BIT_OR) lirAXO(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_BIT_SHL || irinst->opcode == OPCODE_BIT_ASHR || irinst->opcode == OPCODE_BIT_LSHR) lirSAL(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_ALLOCA) liralloca(block, irinst, ctx);
  else if (irinst->opcode == OPCODE_RET) lirret(block, irinst, ctx);
}
