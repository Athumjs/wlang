#include "expr.h"
#include <stdio.h>
#include <utils/numeric.h>
#include <string.h>

struct IRExpr irStructExpr(struct Expr *expr, struct MIRContext *ctx) {
  struct IRType type = ctx->module->globals[expr->expr_struct.symbol->index].value.type;
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, NULL, 1, 0, ctx->block, ctx->arena);
  inst->elem_type = type;

  for (int i = 0; i < expr->expr_struct.fields_len; i++) {
    struct IRInstruction *field = newInst(OPCODE_GEP, NULL, 1, 3, ctx->block, ctx->arena);
    field->elem_type = type;
    addOperandRef(2, inst->vl, inst->elem_type, field);

    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    struct Symbol *symbol = hashmap_get(expr->expr_struct.symbol->symbol_struct.items, &expr->expr_struct.fields[i].name);
    field->operands[field->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = expr->expr_struct.fields[i].index
      }
    };

    struct IRExpr expr_o = irExpr(expr->expr_struct.fields[i].expr, ctx);
    ctx->block = expr_o.block;
    struct IRInstruction *store = newInst(OPCODE_STORE, NULL, 0, 2, ctx->block, ctx->arena);
    store->operands[store->operands_len++] = expr_o.operand;
    addOperandRef(2, field->vl, field->type, store);
  }

  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

void fillArray(int64_t vl, struct IRType *type, struct Expr *expr, struct MIRContext *ctx) {
  for (int i = 0; i < expr->expr_array.exprs_len; i++) {
    struct IRInstruction *gep = newInst(OPCODE_GEP, NULL, 1, 3, ctx->block, ctx->arena);
    gep->elem_type = resolveTypeIR(expr->type, ctx->arena);
    addOperandRef(2, vl, gep->elem_type, gep);

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = 0
      }
    };

    gep->operands[gep->operands_len++] = (struct IROperand){
      .kind = Operand_Constant,
      .constant = (struct IRValue){
        .kind = Value_Int,
        .i = i
      }
    };

    if (expr->expr_array.exprs[i]->kind != Expr_Array) {
      struct IRExpr expr_o = irExpr(expr->expr_array.exprs[i], ctx);
      ctx->block = expr_o.block;
      struct IRInstruction *store = newInst(OPCODE_STORE, NULL, 0, 2, ctx->block, ctx->arena);
      store->operands[store->operands_len++] = expr_o.operand;
      addOperandRef(2, gep->vl, gep->elem_type, store);
    } else fillArray(gep->vl, &gep->type, expr->expr_array.exprs[i], ctx);
  }
}

struct IRExpr irArray(struct Expr *expr, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_ALLOCA, NULL, 1, 0, ctx->block, ctx->arena); 
  inst->elem_type = resolveTypeIR(expr->type, ctx->arena);
  fillArray(inst->vl, &inst->elem_type, expr, ctx);
  return (struct IRExpr){
    .inst = inst,
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = inst->vl
    }
  };
}

struct IRExpr irId(struct Expr *expr, struct MIRContext *ctx) {
  struct Symbol *symbol = expr->expr_identifier.symbol;
  int64_t vl;

  if (symbol->kind == Symbol_Function) {
    return (struct IRExpr){
      .operand = (struct IROperand){
        .kind = Operand_String,
        .string = &symbol->name
      }
    };
  }

  if (symbol->isGlobal) {
    struct IRInstruction *inst = newInst(OPCODE_LOAD, symbol->type, 1, 1, ctx->block, ctx->arena);
    addOperandGlobal(&symbol->name, inst->type, inst);
    vl = inst->vl;
  } else {
    struct IRDef *def = readVariable(ctx->func->ssa, &symbol->name, resolveTypeIR(symbol->type, ctx->arena), ctx->block, ctx->arena);

    if (def->isParam) {
      vl = def->param->vl;
    } else {
      if (def->inst->opcode == OPCODE_ALLOCA && def->inst->elem_type.kind != IRTYPEK_STRUCT && def->inst->elem_type.kind != IRTYPEK_ARRAY) {
        struct IRInstruction *inst = newInst(OPCODE_LOAD, NULL, 1, 1, ctx->block, ctx->arena);
        addOperandRef(2, def->inst->vl, def->inst->elem_type, inst);
        inst->type = def->inst->elem_type;
        vl = inst->vl;
      } else vl = def->inst->vl;
    }
  };

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
      .ref = vl
    }
  };
}

struct IRExpr irThis(struct Expr *expr, struct MIRContext *ctx) {
  struct IRInstruction *inst = newInst(OPCODE_GEP, NULL, 1, 3, ctx->block, ctx->arena);
  inst->elem_type = ctx->module->globals[expr->expr_this.symbol->index].type;
  addOperandRef(2, 0, inst->elem_type, inst);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = 0
    }
  };

  struct Symbol *symbol = hashmap_get(expr->expr_this.symbol->symbol_struct.items, &expr->expr_this.str);

  inst->operands[inst->operands_len++] = (struct IROperand){
    .kind = Operand_Constant,
    .constant = (struct IRValue){
      .kind = Value_Int,
      .i = symbol->index
    }
  };

  struct IRInstruction *load = newInst(OPCODE_LOAD, symbol->type, 1, 1, ctx->block, ctx->arena);
  addOperandRef(2, inst->vl, inst->elem_type, load);

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Register,
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

    struct String *name = arena_alloc(ctx->arena, sizeof(struct String));
    *name = (struct String){
      .length = getNumCase(ctx->module->globals_len) + 2
    };
    char *str = arena_alloc(ctx->arena, name->length);
    snprintf(str, name->length, "g%ld", ctx->module->globals_len);
    name->start = str;

    struct IRGlobal global = (struct IRGlobal){
      .name = name,
      .type = type,
      .value = (struct IROperand){
        .kind = Operand_Constant,
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
        .string = global.name
      }
    };
  }

  return (struct IRExpr){
    .operand = (struct IROperand){
      .kind = Operand_Constant,
      .constant = irNumber(expr)
    }
  };
}
