; RUN: llvm-mc -filetype=obj -triple=arc4-unknown-elf %s -o %t.o
; RUN: llvm-readelf -h %t.o | FileCheck %s

; CHECK: Machine: ARC

  .text
  .globl __start
__start:
  nop
  add r0, r1, r2
