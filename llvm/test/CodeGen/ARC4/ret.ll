; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; All functions must return via j [r31] (jump to blink)

define void @return_void() {
; CHECK-LABEL: return_void:
; CHECK: j [r31]
  ret void
}

define i32 @return_val(i32 %a) {
; CHECK-LABEL: return_val:
; CHECK: j [r31]
  ret i32 %a
}

define i32 @return_add(i32 %a, i32 %b) {
; CHECK-LABEL: return_add:
; CHECK: add r0,
; CHECK: j [r31]
  %c = add i32 %a, %b
  ret i32 %c
}
