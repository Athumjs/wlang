#pragma once

#include <utils/ir.h>
#include <front/parser.h>

struct IRModule {
  struct IRGlobal *globals;
  size_t globals_len;
  size_t globals_cap;
  struct IRFunction init;
  struct IRFunction *functions;
  size_t functions_len;
  size_t functions_cap;
};

void loweringAST(struct Program *program, struct IRModule *ir);
