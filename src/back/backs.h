#pragma once

#include <back/backs.h>
#include <back/lir.h>
#include <back/reg_alloc.h>
#include <stdio.h>

struct Floats {
  struct Float {
    uint8_t is_double;
    double value;
  } *values;
  size_t values_len;
  size_t values_cap;
};

// backs
void x86_64_linux_define(struct Arch *arch);
void x86_64_linux_code(struct IRGlobal *globals, size_t globals_len, struct LIRFunction *funcs, size_t funcs_len, struct Floats *floats, FILE *file, struct Arch *arch);

// lir
void lirgen(struct LIRFunction *func, struct Arch *arch, struct IRFunction *irfunc, struct Arena *arena, struct Live *live, struct Floats *floats);
void showLIR(struct LIRFunction *funcs, size_t funcs_len);

// ssa_destruction.c
void ssa_destruction(struct IRFunction *func, struct Arena *arena);


struct FindInst {
  int64_t block_index;
  int64_t inst_index;
};

// util.c
FILE *create_as(const char *folder_name, const char *filename, char **name, int *name_len);
char *run_assembler(const char *folder_name, const char *name, int name_len, FILE *file);
size_t sizeOf(struct IRType *type);
size_t alignOf(struct IRType *type);
struct IRInstruction *getInst(struct IRFunction *func, int64_t vl);
struct FindInst findInst(struct IRFunction *func, int64_t vl);
