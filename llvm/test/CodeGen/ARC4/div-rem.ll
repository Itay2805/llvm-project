; RUN: llc -march=arc4 < %s | FileCheck %s

; Division and remainder use libcalls (no hardware divider on base ARC4)

define i32 @udiv(i32 %a, i32 %b) {
; CHECK-LABEL: udiv:
; CHECK: bl __udivsi3
  %c = udiv i32 %a, %b
  ret i32 %c
}

define i32 @sdiv(i32 %a, i32 %b) {
; CHECK-LABEL: sdiv:
; CHECK: bl __divsi3
  %c = sdiv i32 %a, %b
  ret i32 %c
}

define i32 @urem(i32 %a, i32 %b) {
; CHECK-LABEL: urem:
; CHECK: bl __umodsi3
  %c = urem i32 %a, %b
  ret i32 %c
}

define i32 @srem(i32 %a, i32 %b) {
; CHECK-LABEL: srem:
; CHECK: bl __modsi3
  %c = srem i32 %a, %b
  ret i32 %c
}
