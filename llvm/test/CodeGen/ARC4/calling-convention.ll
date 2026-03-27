; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Test that arguments arrive in r0-r7 per the ARC4 calling convention

define i32 @return_arg0(i32 %a, i32 %b) {
; CHECK-LABEL: return_arg0:
; CHECK-NOT: add
; CHECK: j [r31]
  ret i32 %a
}

define i32 @return_arg1(i32 %a, i32 %b) {
; CHECK-LABEL: return_arg1:
; CHECK: and r0, r1, r1
; CHECK-NEXT: j [r31]
  ret i32 %b
}

; Test 8-argument function (all in registers)
define i32 @eight_args(i32 %a, i32 %b, i32 %c, i32 %d,
                       i32 %e, i32 %f, i32 %g, i32 %h) {
; CHECK-LABEL: eight_args:
; CHECK: add
; CHECK: j [r31]
  %s1 = add i32 %a, %b
  %s2 = add i32 %c, %d
  %s3 = add i32 %e, %f
  %s4 = add i32 %g, %h
  %s5 = add i32 %s1, %s2
  %s6 = add i32 %s3, %s4
  %r = add i32 %s5, %s6
  ret i32 %r
}

; Test external function call
declare i32 @external(i32, i32)

define i32 @call_extern(i32 %a, i32 %b) {
; CHECK-LABEL: call_extern:
; CHECK: jl [external]
; CHECK: j [r31]
  %r = call i32 @external(i32 %a, i32 %b)
  ret i32 %r
}
