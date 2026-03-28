; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Sign-extend i8/i16 to i32 not yet implemented.
; ARC4 has sexb/sexw instructions that can handle this.

; CHECK: Cannot select
define i32 @sext_i8(i8 %a) {
  %b = sext i8 %a to i32
  ret i32 %b
}
