; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s
; RUN: llvm-mc -triple arc4 -filetype=obj < %s -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck %s --check-prefix=RELOC

; ============================================================
; Branch instructions (opcode 0x04/0x05/0x06)
; Offset is 20-bit signed word displacement relative to delay slot
; ============================================================

; CHECK: b	0 ; encoding: [0x00,0x00,0x00,0x20]
b 0
; CHECK: bl	0 ; encoding: [0x00,0x00,0x00,0x28]
bl 0
; CHECK: lp	0 ; encoding: [0x00,0x00,0x00,0x30]
lp 0

; ============================================================
; Branch with label (PC-relative fixup)
; ============================================================

target:
  nop
; CHECK: b	target ; encoding: [0bA0000000,A,A,0b00100AAA]
; CHECK-NEXT: ;   fixup A - offset: 0, value: target-4, kind: FK_ARC4_Branch20
  b target

; Forward branch
; CHECK: b	forward ; encoding: [0bA0000000,A,A,0b00100AAA]
; CHECK-NEXT: ;   fixup A - offset: 0, value: forward-4, kind: FK_ARC4_Branch20
  b forward
  nop
forward:
  nop

; ============================================================
; Global symbol branch (emits relocation)
; ============================================================

  .globl external_func
; CHECK: b	external_func
  b external_func
; RELOC: .rela.text
; RELOC: external_func

; ============================================================
; Jump register: j [reg]
; ============================================================

; CHECK: j	[blink] ; encoding: [0x00,0x80,0x0f,0x38]
j [blink]
; CHECK: jl	[r10] ; encoding: [0x00,0x02,0x05,0x38]
jl [r10]

; ============================================================
; Jump limm: j limm
; ============================================================

; CHECK: j	4096 ; encoding: [0x00,0x00,0x1f,0x38,0x00,0x04,0x00,0x00]
j 0x1000
; CHECK: jl	8192 ; encoding: [0x00,0x02,0x1f,0x38,0x00,0x08,0x00,0x00]
jl 0x2000
