#include "ir.h"

void irVarLocal(struct Decl *decl, struct MIRContext *ctx) {
  for (int i = 0; i < decl->decl_variable.vars_len; i++) {
    struct Var *var = &decl->decl_variable.vars[i];

    if (var->expr != NULL) {
      struct IRExpr expr_v = irExpr(var->expr, ctx);
      ctx->block = expr_v.block;

      if (var->symbol->isPtr) {
        if (ctx->block->instructions[ctx->block->instructions_len - 1].opcode == OPCODE_LOAD) {
          (*ctx->block->reg)--;
          expr_v.operand.ref = ctx->block->instructions[ctx->block->instructions_len - 1].operands[0].ref;
          ctx->block->instructions_len--;
        }

        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, 1, 1, ctx->block, ctx->arena);
        addOperandType(var->symbol->type, inst, ctx->arena);
        struct IRInstruction *store = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
        store->operands[store->operands_len++] = expr_v.operand;
        addOperandRef(2, inst->vl, inst->operands[0].type, store);
        newDef(var->symbol, ctx->func, ctx->arena);
        setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
        continue;
      }
      
      if (expr_v.operand.kind == Operand_Constant) {
        struct IRInstruction *inst = newInst(OPCODE_CONST, 1, 1, ctx->block, ctx->arena);
        inst->operands[inst->operands_len++] = expr_v.operand;
        expr_v.inst = inst;
      }

      if (var->expr->kind == Expr_Literal && var->expr->expr_literal.kind == LITERAL_STRING) {
        struct IRInstruction *inst = newInst(OPCODE_ALLOCA, 1, 1, ctx->block, ctx->arena);
        addOperandType(var->symbol->type, inst, ctx->arena);
        
        if (var->symbol->type->kind == Type_Array) {
          // struct IRInstruction *memcpy = newInst(OPCODE_MEMCPY, NULL, 0, 3, block, arena);
          // addOperandRef(2, inst->vl, memcpy);
          // memcpy->operands[memcpy->operands_len++] = ref.operand;
          //
          // memcpy->operands[memcpy->operands_len++] = (struct IROperand){
          //   .kind = Operand_Constant,
          //   .constant = (struct IRValue){
          //     .kind = Value_Int,
          //     .i = module->globals[ref.operand.ref].type.array.size
          //   }
          // };
        } else {
          struct IRInstruction *store = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
          store->operands[store->operands_len++] = expr_v.operand;
          addOperandRef(2, inst->vl, inst->operands[0].type, store);
        }

        newDef(var->symbol, ctx->func, ctx->arena);
        setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
      } else {
        if (expr_v.inst == NULL)
          expr_v.inst = &ctx->block->instructions[ctx->block->instructions_len - 1];

        newDef(var->symbol, ctx->func, ctx->arena);
        setCurrentDef(ctx->func->ssa, &var->name, 0, expr_v.inst, ctx->block, ctx->arena);
      }

      continue;
    }

    struct IRInstruction *inst = newInst(OPCODE_ALLOCA, 1, 1, ctx->block, ctx->arena);
    addOperandType(var->symbol->type, inst, ctx->arena);
    newDef(var->symbol, ctx->func, ctx->arena);
    setCurrentDef(ctx->func->ssa, &var->name, 0, inst, ctx->block, ctx->arena);
  }
}

void irDecl(struct Decl *decl, struct MIRContext *ctx) {
  if (decl->kind == Decl_Variable) irVarLocal(decl, ctx);
}
