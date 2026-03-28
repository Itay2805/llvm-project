; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Test loads, stores, and stack frame operations.

define void @store(ptr %p, i32 %v) {
; CHECK-LABEL: store:
; CHECK: st r1, [r0
; CHECK: j [blink]
  store i32 %v, ptr %p
  ret void
}

define i32 @load(ptr %p) {
; CHECK-LABEL: load:
; CHECK: ld r0, [r0
; CHECK: j [blink]
  %v = load i32, ptr %p
  ret i32 %v
}

define i32 @stack_frame(i32 %a, i32 %b) {
; CHECK-LABEL: stack_frame:
; CHECK: sub sp, sp
; CHECK: st
; CHECK: add sp, sp
; CHECK: j [blink]
  %a.addr = alloca i32
  %b.addr = alloca i32
  store i32 %a, ptr %a.addr
  store i32 %b, ptr %b.addr
  %v0 = load i32, ptr %a.addr
  %v1 = load i32, ptr %b.addr
  %r = add i32 %v0, %v1
  ret i32 %r
}
