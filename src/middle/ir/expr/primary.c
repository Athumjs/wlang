#include "expr.h"
#include <string.h>

struct IRExpr irStructExpr(struct Expr *expr, struct MIRContext *ctx) {
  struct IRType type = ctx->module->globals[expr->expr_struct.symbol->index].value.type;
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, 1, 1, ctx->block, ctx->arena);
  inst->operands[inst->operands_len].op_type = IRTYPE_NULL;
  inst->operands[inst->operands_len].kind = Operand_Type;
  inst->operands[inst->operands_len++].type = type;

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct IRInstruction *field = newInst(OPCODE_GEP, 1, 3, ctx->block, ctx->arena);
    addOperandRef(3, inst->vl, inst->operands[0].type, field);

    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = expr->expr_struct.fields[i].index
      }
    };

    struct IRExpr expr_o = irExpr(expr->expr_struct.fields[i].expr, ctx);
    ctx->block = expr_o.block;
    struct IRInstruction *store = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
    store->operands[store->operands_len++] = expr_o.operand;
    addOperandRef(3, field->vl, field->operands[0].op_type, store);
  }

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}

void fillArray(int64_t vl, struct IRType *type, struct Expr *expr, struct MIRContext *ctx) {
  for (int i = 0; i < expr->expr_array.exprs_len; i++) {
    struct IRInstruction *gep = newInst(OPCODE_GEP, 1, 3, ctx->block, ctx->arena);
    addOperandRef(3, vl, *type, gep);

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .op_type.kind = IRTYPEK_OTHER,
      .op_type.other = IRTYPE_i32,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = i
      }
    };

    if (expr->expr_array.exprs[i]->kind != Expr_Array) {
      struct IRExpr expr_o = irExpr(expr->expr_array.exprs[i], ctx);
      ctx->block = expr_o.block;
      struct IRInstruction *store = newInst(OPCODE_STORE, 0, 2, ctx->block, ctx->arena);
      store->operands[store->operands_len++] = expr_o.operand;
      addOperandRef(3, gep->vl, gep->operands[0].op_type, store);
    } else fillArray(gep->vl, &gep->operands[0].op_type, expr->expr_array.exprs[i], ctx);
  }
}

struct IRExpr irArray(struct Expr *expr, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, 1, 0, ctx->block, ctx->arena); 
  addOperandType(expr->type, inst, ctx->arena);
  fillArray(inst->vl, &inst->operands[0].op_type, expr, ctx);
  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = inst->operands[0].op_type,
      .ref = inst->vl
    }
  };
}

struct IRExpr irId(struct Expr *expr, struct MIRContext *ctx) {
  struct Symbol *symbol = expr->expr_identifier.symbol;
  int64_t vl;
  struct IRType type = IRTYPE_NULL;

  if (symbol->kind == Symbol_Function) {
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_String,
        .op_type = resolveTypeIR(symbol->type, ctx->arena),
        .string = &symbol->name
      }
    };
  }

  if (symbol->isGlobal) {
    struct IRGlobal *global = &ctx->module->globals[symbol->index];
    struct IRInstruction *inst = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
    addOperandRef(1, global->vl, resolveTypeIR(symbol->type, ctx->arena), inst);
    vl = inst->vl;
    type = inst->operands[0].op_type;
  } else {
    struct IRDef *def = getCurrentDef(ctx->func->ssa, &symbol->name, ctx->block, ctx->arena);

    if (def->isParam) {
      vl = def->param->vl;
      type = def->param->type;
    } else {
      if (def->inst->opcode == OPCODE_ALLOCA) {
        struct IRInstruction *inst = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
        addOperandRef(3, def->inst->vl, def->inst->operands[0].op_type, inst);
        vl = inst->vl;
        type = inst->operands[0].op_type;
      } else {
        vl = def->inst->vl;
        type = def->inst->operands[0].op_type;
      }
    }
  };

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = type,
      .ref = vl
    }
  };
}

struct IRExpr irThis(struct Expr *expr, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_GEP, 1, 3, ctx->block, ctx->arena);
  addOperandRef(3, 0, IRTYPE_NULL, inst);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .op_type.kind = IRTYPEK_OTHER,
    .op_type.other = IRTYPE_i32,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  struct Symbol *symbol = hashmap_get(expr->expr_this.symbol->symbol_struct.items, &expr->expr_this.str);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .op_type.kind = IRTYPEK_OTHER,
    .op_type.other = IRTYPE_i32,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = symbol->index
    }
  };

  struct IRInstruction *load = newInst(OPCODE_LOAD, 1, 1, ctx->block, ctx->arena);
  addOperandRef(3, inst->vl, inst->operands[0].op_type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .op_type = load->operands[0].op_type,
      .ref = load->vl
    }
  };
}

struct IRExpr irLiteral(struct Expr *expr, struct MIRContext *ctx) {
  if (expr->expr_literal.kind == LITERAL_STRING) {
    if (ctx->module->globals_len == ctx->module->globals_cap) {
      size_t oldCap = ctx->module->globals_cap;
      ctx->module->globals_cap *= 2;
      struct IRGlobal *temp = arena_alloc(ctx->arena, ctx->module->globals_cap * sizeof(struct IRGlobal));
      memcpy(temp, ctx->module->globals, oldCap * sizeof(struct IRGlobal));
      ctx->module->globals = temp;
    }

    struct IRType *typebase = arena_alloc(ctx->arena, sizeof(struct IRType));
    typebase->kind = IRTYPEK_OTHER;
    typebase->other = IRTYPE_i8;
    struct IRType type;
    type.kind = IRTYPEK_ARRAY;
    type.array.type = typebase;
    type.array.size = expr->expr_literal.literal.string.length + 1;

    struct IRGlobal global = (struct IRGlobal){
      .vl = ctx->module->globals_len,
      .value = (struct IROperand){
        .kind = Operand_Constant,
        .op_type = type,
        .constant = (struct IRValue){
          .kind = Value_String,
          .s = &expr->expr_literal.literal.string
        }
      }
    };

    ctx->module->globals[ctx->module->globals_len++] = global;
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_Global,
        .op_type = global.value.op_type,
        .ref = global.vl
      }
    };
  }

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Constant,
      .op_type = resolveTypeIR(expr->type, ctx->arena),
      .constant = irNumber(expr)
    }
  };
}
