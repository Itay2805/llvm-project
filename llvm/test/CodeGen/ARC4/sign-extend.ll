; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Sign/zero extension operations

define i32 @sext_i8(i8 %a) {
; CHECK-LABEL: sext_i8:
; CHECK: sexb r0, r0
  %b = sext i8 %a to i32
  ret i32 %b
}

define i32 @sext_i16(i16 %a) {
; CHECK-LABEL: sext_i16:
; CHECK: sexw r0, r0
  %b = sext i16 %a to i32
  ret i32 %b
}

define i32 @zext_i8(i8 %a) {
; CHECK-LABEL: zext_i8:
; CHECK: and r0, r0,
  %b = zext i8 %a to i32
  ret i32 %b
}

define i32 @sext_add(i8 %a, i32 %b) {
; CHECK-LABEL: sext_add:
; CHECK: sexb r{{[0-9]+}}, r0
; CHECK: add r0, r{{[0-9]+}}, r1
  %x = sext i8 %a to i32
  %y = add i32 %x, %b
  ret i32 %y
}
