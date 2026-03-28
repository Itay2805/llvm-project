; RUN: llc -march=arc4 -verify-machineinstrs < %s | FileCheck %s

; Test that the branch analyzer correctly handles complex control flow
; patterns, including diamond, triangle, and chained branches.

; Diamond CFG: exercises the cond+uncond branch pair analysis.
; The branch analyzer must correctly identify both the conditional and
; unconditional targets to allow proper block placement.
define i32 @diamond(i32 %a, i32 %b) {
; CHECK-LABEL: diamond:
; CHECK:      sub.f 0, r0, r1
; CHECK:      b.
; CHECK:      mov r0,
; CHECK:      j [blink]
; CHECK:      mov r0,
; CHECK:      j [blink]
entry:
  %c = icmp sgt i32 %a, %b
  br i1 %c, label %then, label %else
then:
  ret i32 1
else:
  ret i32 0
}

; Triangle CFG: conditional branch with fallthrough.
define i32 @triangle(i32 %a, i32 %b) {
; CHECK-LABEL: triangle:
; CHECK:      sub.f 0, r0, r1
; CHECK:      b.
entry:
  %c = icmp sgt i32 %a, %b
  br i1 %c, label %then, label %end
then:
  %x = add i32 %a, 1
  br label %end
end:
  %r = phi i32 [%x, %then], [%a, %entry]
  ret i32 %r
}

; Chained comparisons: multiple conditional branches in sequence.
; Each block with a cond+uncond pair must be correctly analyzed.
define i32 @chained(i32 %a, i32 %b) {
; CHECK-LABEL: chained:
; CHECK:      sub.f
; CHECK:      b.
; CHECK:      sub.f
; CHECK:      b.
entry:
  %c1 = icmp sgt i32 %a, %b
  br i1 %c1, label %mid, label %else
mid:
  %c2 = icmp eq i32 %a, 0
  br i1 %c2, label %then, label %else
then:
  ret i32 2
else:
  ret i32 0
}

; Loop with branch reversal: the back-edge branch condition may need
; to be reversed during block placement. Tests reverseBranchCondition.
define i32 @loop_with_reversal(i32 %n) {
; CHECK-LABEL: loop_with_reversal:
; CHECK:      sub.f
; CHECK:      b.
; CHECK:      j [blink]
entry:
  br label %loop
loop:
  %i = phi i32 [0, %entry], [%i2, %body]
  %c = icmp slt i32 %i, %n
  br i1 %c, label %body, label %exit
body:
  %i2 = add i32 %i, 1
  br label %loop
exit:
  ret i32 %i
}

; Nested diamonds: tests that branch analysis works correctly when
; multiple diamond patterns are nested.
define i32 @nested_diamond(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: nested_diamond:
; CHECK:      sub.f
; CHECK:      b.
; CHECK:      sub.f
; CHECK:      b.
entry:
  %c1 = icmp sgt i32 %a, %b
  br i1 %c1, label %outer_then, label %outer_else
outer_then:
  %c2 = icmp eq i32 %a, %c
  br i1 %c2, label %inner_then, label %inner_else
inner_then:
  br label %end
inner_else:
  %x = add i32 %a, 1
  br label %end
outer_else:
  %y = add i32 %b, 1
  br label %end
end:
  %r = phi i32 [%a, %inner_then], [%x, %inner_else], [%y, %outer_else]
  ret i32 %r
}

; Unsigned comparison branch reversal: tests that all condition code
; inversions work correctly, not just signed ones.
define i32 @unsigned_cmp(i32 %a, i32 %b) {
; CHECK-LABEL: unsigned_cmp:
; CHECK:      sub.f 0, r0, r1
; CHECK:      b.
entry:
  %c = icmp ugt i32 %a, %b
  br i1 %c, label %then, label %else
then:
  ret i32 1
else:
  ret i32 0
}
