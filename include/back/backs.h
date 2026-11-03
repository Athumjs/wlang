#pragma once

#include <middle/irgen.h>

struct Register {
  int64_t id;
  uint8_t reserve;
  uint8_t size;
};

struct Arch {
  const struct Register *registers;
  size_t iregisters_len;
  size_t fregisters_len;

  const int64_t *arg_registers;
  size_t arg_registers_len;

  uint8_t is_3_operands;

  int64_t ret_register;
  int64_t stack_register;
  int64_t inst_register;
  int64_t div_register;
  int64_t rem_register;
};

char *backs(struct IRModule *ir, struct Program *program, const char *folder_name, uint8_t only_asm, uint8_t show_lir);
