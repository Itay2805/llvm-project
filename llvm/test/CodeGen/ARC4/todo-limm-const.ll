; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Large constants (outside shimm9 range) not yet implemented.
; Needs limm materialization pattern.

; CHECK: Constants outside simm9 range not yet supported
define i32 @big_const() {
  ret i32 100000
}
