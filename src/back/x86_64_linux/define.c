#include "../backs.h"

void x86_64_linux_define(struct Arch *arch) {
  static struct Register regs[32] = {
//  rax        rbx        rcx        rdx
    {0, 0, 8}, {1, 0, 8}, {2, 0, 8}, {3, 0, 8},
//  rsi        rdi        rbp        rsp
    {4, 0, 8}, {5, 0, 8}, {6, 1, 8}, {7, 1, 8},
//  r8         r9         r10         r11
    {8, 0, 8}, {9, 0, 8}, {10, 0, 8}, {11, 0, 8},
//  r12         r13         r14         r15
    {12, 0, 8}, {13, 0, 8}, {14, 0, 8}, {15, 0, 8},
//  xmm0        xmm1        xmm2        xmm3
    {16, 0, 8}, {17, 0, 8}, {18, 0, 8}, {19, 0, 8},
//  xmm4        xmm5        xmm6        xmm7
    {20, 0, 8}, {21, 0, 8}, {22, 0, 8}, {23, 0, 8},
//  xmm8        xmm9        xmm10       xmm11
    {24, 0, 8}, {25, 0, 8}, {26, 0, 8}, {27, 0, 8},
//  xmm12       xmm13       xmm14       xmm15
    {28, 0, 8}, {29, 0, 8}, {30, 0, 8}, {31, 0, 8},
  };

  static const int64_t args[6] = {5, 4, 3, 2, 8, 9};
  static const int64_t callee_saved[5] = {1, 12, 13, 14, 15};

  *arch = (struct Arch){
    .registers = regs,
    .iregisters_len = 16,
    .fregisters_len = 16,

    .arg_registers = args,
    .arg_registers_len = 6,

    .callee_saved_registers = callee_saved,
    .callee_saved_registers_len = 5,

    .is_3_operands = 0,

    .ret_register = 0,
    .stack_register = 6,
    .inst_register = 100,
    .div_register = 0,
    .rem_register = 3
  };
}
