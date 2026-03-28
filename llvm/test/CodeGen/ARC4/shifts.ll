; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Shift operations expand to libcalls since ARC4 base ISA lacks
; a barrel shifter.

; CHECK-LABEL: shl:
; CHECK: bl __ashlsi3
define i32 @shl(i32 %a, i32 %b) {
  %c = shl i32 %a, %b
  ret i32 %c
}
