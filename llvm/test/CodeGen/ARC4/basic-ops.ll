; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Verify all basic C operations compile to valid ARC4 assembly.

define i32 @add(i32 %a, i32 %b) {
; CHECK-LABEL: add:
; CHECK: add r0,
; CHECK: j [blink]
  %r = add i32 %a, %b
  ret i32 %r
}

define i32 @sub(i32 %a, i32 %b) {
; CHECK-LABEL: sub:
; CHECK: sub r0,
; CHECK: j [blink]
  %r = sub i32 %a, %b
  ret i32 %r
}

define i32 @neg(i32 %a) {
; CHECK-LABEL: neg:
; CHECK: sub r0,
; CHECK: j [blink]
  %r = sub i32 0, %a
  ret i32 %r
}

define i32 @shl2(i32 %a) {
; CHECK-LABEL: shl2:
; CHECK: add r0, r0, r0
; CHECK: add r0, r0, r0
; CHECK: j [blink]
  %r = shl i32 %a, 2
  ret i32 %r
}

define i32 @sra2(i32 %a) {
; CHECK-LABEL: sra2:
; CHECK: asr
; CHECK: asr
; CHECK: j [blink]
  %r = ashr i32 %a, 2
  ret i32 %r
}

define i32 @srl1(i32 %a) {
; CHECK-LABEL: srl1:
; CHECK: lsr
; CHECK: j [blink]
  %r = lshr i32 %a, 1
  ret i32 %r
}

; Variable shifts use library calls
define i32 @shl_var(i32 %a, i32 %b) {
; CHECK-LABEL: shl_var:
; CHECK: jl __ashlsi3
; CHECK: j [blink]
  %r = shl i32 %a, %b
  ret i32 %r
}

define i32 @sra_var(i32 %a, i32 %b) {
; CHECK-LABEL: sra_var:
; CHECK: jl __ashrsi3
; CHECK: j [blink]
  %r = ashr i32 %a, %b
  ret i32 %r
}

define i32 @srl_var(i32 %a, i32 %b) {
; CHECK-LABEL: srl_var:
; CHECK: jl __lshrsi3
; CHECK: j [blink]
  %r = lshr i32 %a, %b
  ret i32 %r
}
