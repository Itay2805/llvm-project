; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Multiply expands to libcall since base ARC4 has no hardware multiplier.

; CHECK-LABEL: mul:
; CHECK: bl __mulsi3
define i32 @mul(i32 %a, i32 %b) {
  %c = mul i32 %a, %b
  ret i32 %c
}
