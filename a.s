// wlang
.section .text
.globl _start
main:
  pushq %rbp
  movq %rsp, %rbp
  movl $21, %eax
  movl $10, %ebx
  movl $2, %ecx
  movl %eax, %edx
  addl $9, %edx
  movl %eax, -4(%rbp)
  movl %ebx, %eax
  movl %edx, -8(%rbp)
  cltd
  idivl %ecx
  movl %eax, %ebx
  imull -8(%rbp), %ebx
  movl %ebx, %eax
  popq %rbp
  ret
_start:
  call main
  movl %eax, %edi
  movl $60, %eax
  syscall
