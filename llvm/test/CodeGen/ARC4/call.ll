; RUN: llc -march=arc4 < %s | FileCheck %s

; Function calls

declare i32 @foo()
declare i32 @bar(i32, i32)
declare void @use(i32)

define i32 @call_no_args() {
; CHECK-LABEL: call_no_args:
; CHECK: st blink, [sp
; CHECK: bl foo
; CHECK: ld blink, [sp
; CHECK: j [blink]
  %r = call i32 @foo()
  ret i32 %r
}

define i32 @call_with_args(i32 %a, i32 %b) {
; CHECK-LABEL: call_with_args:
; CHECK: bl bar
  %r = call i32 @bar(i32 %a, i32 %b)
  ret i32 %r
}

define i32 @callee_saved_across_call(i32 %a) {
; CHECK-LABEL: callee_saved_across_call:
; CHECK: st blink
; CHECK: mov r13, r0
; CHECK: bl use
; CHECK: mov r0, r13
; CHECK: ld blink
; CHECK: j [blink]
  call void @use(i32 %a)
  ret i32 %a
}

define i32 @four_args(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: four_args:
; CHECK: add
; CHECK: add
; CHECK: add r0
  %x = add i32 %a, %b
  %y = add i32 %c, %d
  %z = add i32 %x, %y
  ret i32 %z
}

define i32 @eight_args(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i32 %h) {
; CHECK-LABEL: eight_args:
; CHECK: add r0, r0, r7
  %x = add i32 %a, %h
  ret i32 %x
}
