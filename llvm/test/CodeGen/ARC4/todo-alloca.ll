; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Stack-allocated local variables (alloca) not yet implemented.
; Needs FrameIndex selection in ISelDAGToDAG.

; CHECK: Cannot select
define i32 @local_var() {
  %p = alloca i32
  store i32 42, ptr %p
  %v = load i32, ptr %p
  ret i32 %v
}
