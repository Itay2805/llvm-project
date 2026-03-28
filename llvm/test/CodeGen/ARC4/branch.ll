; RUN: llc -march=arc4 < %s | FileCheck %s

; === Conditional branches ===

define i32 @br_sgt(i32 %a, i32 %b) {
; CHECK-LABEL: br_sgt:
; CHECK: sub.f 0
; CHECK: b.
  %c = icmp sgt i32 %a, %b
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}

define i32 @br_eq(i32 %a, i32 %b) {
; CHECK-LABEL: br_eq:
; CHECK: sub.f 0
  %c = icmp eq i32 %a, %b
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}

define i32 @br_ne(i32 %a, i32 %b) {
; CHECK-LABEL: br_ne:
; CHECK: sub.f 0
  %c = icmp ne i32 %a, %b
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}

define i32 @br_slt(i32 %a, i32 %b) {
; CHECK-LABEL: br_slt:
; CHECK: sub.f 0
  %c = icmp slt i32 %a, %b
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}

define i32 @br_uge(i32 %a, i32 %b) {
; CHECK-LABEL: br_uge:
; CHECK: sub.f 0
  %c = icmp uge i32 %a, %b
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}

; === Select (conditional move) ===

define i32 @select_max(i32 %a, i32 %b) {
; CHECK-LABEL: select_max:
; CHECK: sub.f 0
  %c = icmp sgt i32 %a, %b
  %r = select i1 %c, i32 %a, i32 %b
  ret i32 %r
}

define i32 @select_min(i32 %a, i32 %b) {
; CHECK-LABEL: select_min:
; CHECK: sub.f 0
  %c = icmp slt i32 %a, %b
  %r = select i1 %c, i32 %a, i32 %b
  ret i32 %r
}

; === Loops ===

define i32 @loop_sum(i32 %n) {
; CHECK-LABEL: loop_sum:
; CHECK: sub.f
; CHECK: b.
entry:
  br label %loop
loop:
  %i = phi i32 [0, %entry], [%i2, %loop]
  %s = phi i32 [0, %entry], [%s2, %loop]
  %s2 = add i32 %s, %i
  %i2 = add i32 %i, 1
  %c = icmp slt i32 %i2, %n
  br i1 %c, label %loop, label %exit
exit:
  ret i32 %s2
}

define i32 @countdown(i32 %n) {
; CHECK-LABEL: countdown:
; CHECK: sub.f
; CHECK: b.
entry:
  br label %loop
loop:
  %i = phi i32 [%n, %entry], [%i2, %loop]
  %i2 = sub i32 %i, 1
  %c = icmp sgt i32 %i2, 0
  br i1 %c, label %loop, label %exit
exit:
  ret i32 %i2
}

; === Compare with zero ===

define i32 @br_zero(i32 %a) {
; CHECK-LABEL: br_zero:
; CHECK: sub.f 0
  %c = icmp eq i32 %a, 0
  br i1 %c, label %t, label %f
t:
  ret i32 1
f:
  ret i32 0
}
