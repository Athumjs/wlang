#include "ir.h"

void irVarLocal(struct Decl *decl, struct MIRContext *ctx) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];

    if (var->expr != NULL) {
      struct IRExpr expr_v = irExpr(var->expr, ctx);
      ctx->block = expr_v.block;

      if (var->symbol->needAlloca) {
        if (ctx->block->instructions[ctx->block->instructions_len - 1].opcode == OPCODE_LOAD) {
          (*ctx->block->reg)--;
          expr_v.operand.ref = ctx->block->instructions[ctx->block->instructions_len - 1].operands[0].ref;
          ctx->block->instructions_len--;
        }

        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, NULL, 1, 0, ctx->block, ctx);
        inst->elem_type = resolveTypeIR(ctx->module, var->symbol->type, ctx->arena);
        struct IRInstruction *store = newInst(OPCODE_STORE, NULL, 0, 2, ctx->block, ctx);
        store->operands[store->operands_len++] = expr_v.operand;
        addOperandRef(2, inst->vl, inst->elem_type, store);
        setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
        continue;
      }
      
      if (expr_v.operand.kind == Operand_Constant) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, var->symbol->type, 1, 1, ctx->block, ctx);
        inst->operands[inst->operands_len++] = expr_v.operand;
        expr_v.inst = inst;
      }

      if (var->expr->kind == Expr_Literal && var->expr->expr_literal.kind == LITERAL_STRING) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, NULL, 1, 1, ctx->block, ctx);
        inst->type = ctx->module->globals[ctx->module->globals_len - 1].type;
        inst->operands[inst->operands_len++] = expr_v.operand;
        setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
      } else {
        if (expr_v.inst == NULL)
          expr_v.inst = &ctx->block->instructions[ctx->block->instructions_len - 1];

        setCurrentDef(ctx->func->ssa, &var->name, 0, expr_v.inst, ctx->block, ctx->arena);
      }

      continue;
    }

    struct IRInstruction *inst = newInst(OPCODE_ALLOCA, NULL, 1, 0, ctx->block, ctx);
    inst->elem_type = resolveTypeIR(ctx->module, var->symbol->type, ctx->arena);
    setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
  }
}

void irDecl(struct Decl *decl, struct MIRContext *ctx) {
  if (decl->kind == Decl_Variable) irVarLocal(decl, ctx);
}
