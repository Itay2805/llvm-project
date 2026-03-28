; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Stack-allocated local variables

define i32 @local_i32() {
; CHECK-LABEL: local_i32:
; CHECK: sub sp, sp,
; CHECK: mov r{{[0-9]+}}, 42
; CHECK: st r{{[0-9]+}},
; CHECK: add sp, sp,
; CHECK: j [blink]
  %p = alloca i32
  store i32 42, ptr %p
  %v = load i32, ptr %p
  ret i32 %v
}

define i32 @two_locals() {
; CHECK-LABEL: two_locals:
; CHECK: sub sp, sp,
; CHECK: st
; CHECK: st
; CHECK: mov r0, 30
; CHECK: add sp, sp,
  %a = alloca i32
  %b = alloca i32
  store i32 10, ptr %a
  store i32 20, ptr %b
  %va = load i32, ptr %a
  %vb = load i32, ptr %b
  %r = add i32 %va, %vb
  ret i32 %r
}

define void @local_write_arg(i32 %v) {
; CHECK-LABEL: local_write_arg:
; CHECK: sub sp, sp,
; CHECK: st r0,
; CHECK: add sp, sp,
; CHECK: j [blink]
  %p = alloca i32
  store i32 %v, ptr %p
  ret void
}
