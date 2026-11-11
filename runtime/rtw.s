// runtime wlang
.section .text
.globl _start

_start:
  call main
  movl %eax, %edi
  movl $60, %eax
  syscall
