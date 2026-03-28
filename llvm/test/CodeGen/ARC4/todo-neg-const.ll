; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Negative constants crash with APInt assertion.
; The shimm constant materialization doesn't handle negative values correctly.

; CHECK: Assertion
define i32 @neg1() {
  ret i32 -1
}
