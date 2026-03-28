; RUN: llc -march=arc4 < %s | FileCheck %s

; Basic ALU operations

define i32 @ret42() {
; CHECK-LABEL: ret42:
; CHECK: mov r0, 42
; CHECK: j [blink]
  ret i32 42
}

define i32 @identity(i32 %a) {
; CHECK-LABEL: identity:
; CHECK: j [blink]
  ret i32 %a
}

define i32 @add(i32 %a, i32 %b) {
; CHECK-LABEL: add:
; CHECK: add r0, r0, r1
; CHECK: j [blink]
  %c = add i32 %a, %b
  ret i32 %c
}

define i32 @add_const(i32 %a) {
; CHECK-LABEL: add_const:
; CHECK: mov r{{[0-9]+}}, 5
; CHECK: add r0, r0, r{{[0-9]+}}
  %c = add i32 %a, 5
  ret i32 %c
}

define i32 @sub(i32 %a, i32 %b) {
; CHECK-LABEL: sub:
; CHECK: sub r0, r0, r1
  %c = sub i32 %a, %b
  ret i32 %c
}

define i32 @and_op(i32 %a, i32 %b) {
; CHECK-LABEL: and_op:
; CHECK: and r0, r0, r1
  %c = and i32 %a, %b
  ret i32 %c
}

define i32 @or_op(i32 %a, i32 %b) {
; CHECK-LABEL: or_op:
; CHECK: or r0, r0, r1
  %c = or i32 %a, %b
  ret i32 %c
}

define i32 @xor_op(i32 %a, i32 %b) {
; CHECK-LABEL: xor_op:
; CHECK: xor r0, r0, r1
  %c = xor i32 %a, %b
  ret i32 %c
}

define i32 @zext_i8(i8 %a) {
; CHECK-LABEL: zext_i8:
; CHECK: and r0, r0, r{{[0-9]+}}
  %b = zext i8 %a to i32
  ret i32 %b
}
