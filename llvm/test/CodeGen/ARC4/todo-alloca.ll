; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Stack-allocated local variables (alloca) use FrameIndex selection.

; CHECK-LABEL: local_var:
; CHECK: sub sp, sp,
; CHECK: mov r0, 42
; CHECK: st r0,
; CHECK: add sp, sp,
; CHECK: j [blink]
define i32 @local_var() {
  %p = alloca i32
  store i32 42, ptr %p
  %v = load i32, ptr %p
  ret i32 %v
}
