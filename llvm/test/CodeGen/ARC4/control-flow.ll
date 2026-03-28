; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Verify conditional branches have proper condition code suffixes.

define i32 @max(i32 %a, i32 %b) {
; CHECK-LABEL: max:
; CHECK: b.gt
; CHECK: j [blink]
  %cmp = icmp sgt i32 %a, %b
  %r = select i1 %cmp, i32 %a, i32 %b
  ret i32 %r
}

define i32 @if_else(i32 %a) {
; CHECK-LABEL: if_else:
; CHECK: b.{{gt|lt|le|ge}}
; CHECK: j [blink]
  %cmp = icmp sgt i32 %a, 0
  br i1 %cmp, label %then, label %else

then:
  ret i32 1

else:
  ret i32 0
}

define i32 @eq_test(i32 %a, i32 %b) {
; CHECK-LABEL: eq_test:
; CHECK: b.eq
; CHECK: j [blink]
  %cmp = icmp eq i32 %a, %b
  %r = select i1 %cmp, i32 1, i32 0
  ret i32 %r
}

; While loop at O0 (avoid optimizer turning it into multiply)
; RUN: llc -mtriple=arc4 -O0 < %s | FileCheck %s --check-prefix=LOOP

define i32 @while_loop(i32 %n) {
; LOOP-LABEL: while_loop:
; LOOP: sub.f
; LOOP: b.lt
; LOOP: b
; LOOP: j [blink]
entry:
  %n.addr = alloca i32
  %s = alloca i32
  store i32 %n, ptr %n.addr
  store i32 0, ptr %s
  br label %loop

loop:
  %nv = load i32, ptr %n.addr
  %cmp = icmp sgt i32 %nv, 0
  br i1 %cmp, label %body, label %end

body:
  %sv = load i32, ptr %s
  %nv2 = load i32, ptr %n.addr
  %add = add i32 %sv, %nv2
  store i32 %add, ptr %s
  %dec = sub i32 %nv2, 1
  store i32 %dec, ptr %n.addr
  br label %loop

end:
  %result = load i32, ptr %s
  ret i32 %result
}
