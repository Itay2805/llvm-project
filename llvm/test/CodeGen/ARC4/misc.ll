; RUN: llc -march=arc4 < %s | FileCheck %s

; Miscellaneous patterns

; === Switch statement ===
define i32 @switch_test(i32 %a) {
; CHECK-LABEL: switch_test:
; CHECK: sub.f
; CHECK: b.
  switch i32 %a, label %d [i32 0, label %z i32 1, label %o]
z:
  ret i32 10
o:
  ret i32 20
d:
  ret i32 30
}

; === Void call ===
declare void @side_effect()

define void @call_void() {
; CHECK-LABEL: call_void:
; CHECK: bl side_effect
  call void @side_effect()
  ret void
}

; === Call with 9+ args (stack args) ===
declare i32 @many_args(i32,i32,i32,i32,i32,i32,i32,i32,i32)

define i32 @call_stack_args(i32 %a) {
; CHECK-LABEL: call_stack_args:
; CHECK: st r0, [sp
; CHECK: bl many_args
  %r = call i32 @many_args(i32 %a, i32 %a, i32 %a, i32 %a, i32 %a, i32 %a, i32 %a, i32 %a, i32 %a)
  ret i32 %r
}

; === Shift by constant 1 ===
define i32 @shl_1(i32 %a) {
; CHECK-LABEL: shl_1:
; CHECK: bl __ashlsi3
  %c = shl i32 %a, 1
  ret i32 %c
}

; === Truncate (should be no-op) ===
define i8 @trunc_i8(i32 %a) {
; CHECK-LABEL: trunc_i8:
; CHECK: j [blink]
  %b = trunc i32 %a to i8
  ret i8 %b
}

define i16 @trunc_i16(i32 %a) {
; CHECK-LABEL: trunc_i16:
; CHECK: j [blink]
  %b = trunc i32 %a to i16
  ret i16 %b
}

; === Large constant (i32 max) ===
define i32 @const_max() {
; CHECK-LABEL: const_max:
; CHECK: or r0,
  ret i32 2147483647
}

; === Volatile load ===
define i32 @volatile_load(ptr %p) {
; CHECK-LABEL: volatile_load:
; CHECK: ld r0, [r0]
  %v = load volatile i32, ptr %p
  ret i32 %v
}

; === Alloca array ===
define void @alloca_array() {
; CHECK-LABEL: alloca_array:
; CHECK: sub sp, sp,
  %p = alloca [4 x i32]
  ret void
}

; === Unreachable ===
define void @unreachable_fn() {
; CHECK-LABEL: unreachable_fn:
  unreachable
}

; === PHI node ===
define i32 @phi_test(i1 %c) {
; CHECK-LABEL: phi_test:
  br i1 %c, label %t, label %e
t:
  br label %m
e:
  br label %m
m:
  %r = phi i32 [1, %t], [0, %e]
  ret i32 %r
}
