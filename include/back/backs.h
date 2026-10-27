#pragma once

#include <middle/irgen.h>

struct Back {
  char *obj_name;
  char *folder_name;
};

struct Back backs(struct IRModule *ir, struct Program *program, uint8_t only_asm);
