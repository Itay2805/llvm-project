; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Shift operations not yet implemented.
; ARC4 base ISA has single-bit shifts (asl/asr/lsr).
; Multi-bit shifts need barrel shifter extension or expansion.

; CHECK: Unable to legalize non-vector shift
define i32 @shl(i32 %a, i32 %b) {
  %c = shl i32 %a, %b
  ret i32 %c
}
