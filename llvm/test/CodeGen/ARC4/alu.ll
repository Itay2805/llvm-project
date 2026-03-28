; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

define i32 @add_rr(i32 %a, i32 %b) {
; CHECK-LABEL: add_rr:
; CHECK: add r0, {{r[01]}}, {{r[01]}}
; CHECK-NEXT: j [blink]
  %c = add i32 %a, %b
  ret i32 %c
}

define i32 @sub_rr(i32 %a, i32 %b) {
; CHECK-LABEL: sub_rr:
; CHECK: sub r0, r0, r1
; CHECK-NEXT: j [blink]
  %c = sub i32 %a, %b
  ret i32 %c
}

define i32 @and_rr(i32 %a, i32 %b) {
; CHECK-LABEL: and_rr:
; CHECK: and r0, {{r[01]}}, {{r[01]}}
; CHECK-NEXT: j [blink]
  %c = and i32 %a, %b
  ret i32 %c
}

define i32 @or_rr(i32 %a, i32 %b) {
; CHECK-LABEL: or_rr:
; CHECK: or r0, {{r[01]}}, {{r[01]}}
; CHECK-NEXT: j [blink]
  %c = or i32 %a, %b
  ret i32 %c
}

define i32 @xor_rr(i32 %a, i32 %b) {
; CHECK-LABEL: xor_rr:
; CHECK: xor r0, {{r[01]}}, {{r[01]}}
; CHECK-NEXT: j [blink]
  %c = xor i32 %a, %b
  ret i32 %c
}

define i32 @add_imm(i32 %a) {
; CHECK-LABEL: add_imm:
; CHECK: add r0, r0, 42
; CHECK-NEXT: j [blink]
  %c = add i32 %a, 42
  ret i32 %c
}

define i32 @chain(i32 %a, i32 %b) {
; CHECK-LABEL: chain:
; CHECK-DAG: and
; CHECK-DAG: sub
; CHECK: or
; CHECK: j [blink]
  %x = and i32 %a, %b
  %y = sub i32 %a, %b
  %z = or i32 %x, %y
  ret i32 %z
}
