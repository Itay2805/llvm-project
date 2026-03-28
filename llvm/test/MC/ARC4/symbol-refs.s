; RUN: llvm-mc -filetype=obj -triple=arc4-unknown-elf %s -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck %s

; Test that symbol references produce correct relocations.
; Reloc type values: 5 = R_ARC_B26, 6 = R_ARC_B22_PCREL
; (readelf shows "Unknown" since ARC4-specific reloc names aren't registered)

; b and bl use B22_PCREL (type 6) — PC-relative, word-aligned
; CHECK: 00000106
; CHECK: 00000106

; jl limm uses B26 (type 5) — absolute, word-aligned
; CHECK: 00000105

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
