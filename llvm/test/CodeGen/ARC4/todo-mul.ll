; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s

; TODO: Multiply not available on base ARC4 (no hardware multiplier).
; Needs libcall expansion or mul64 extension.

; CHECK: no libcall available for mul
define i32 @mul(i32 %a, i32 %b) {
  %c = mul i32 %a, %b
  ret i32 %c
}
