; RUN: llvm-mc -filetype=obj -triple=arc4-unknown-elf %s -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck %s

; Test that symbol references produce correct relocations.

; b and bl use R_ARC_B22_PCREL — PC-relative, word-aligned
; CHECK: R_ARC_B22_PCREL
; CHECK: R_ARC_B22_PCREL

; jl limm uses R_ARC_B26 — absolute, word-aligned
; CHECK: R_ARC_B26

  .text
  .globl __start
__start:
  b target
  bl target
  jl func
  add r0, r1, r2
target:
  nop
func:
  j [r31]
