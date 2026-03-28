; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Sign-extend i8/i16 to i32 using sexb/sexw instructions.

; CHECK-LABEL: sext_i8:
; CHECK: sexb r0, r0
; CHECK: j [blink]
define i32 @sext_i8(i8 %a) {
  %b = sext i8 %a to i32
  ret i32 %b
}
