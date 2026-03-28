; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; === Shimm constants (-256 to 255) ===

define i32 @const_0() {
; CHECK-LABEL: const_0:
; CHECK: mov r0, 0
  ret i32 0
}

define i32 @const_1() {
; CHECK-LABEL: const_1:
; CHECK: mov r0, 1
  ret i32 1
}

define i32 @const_42() {
; CHECK-LABEL: const_42:
; CHECK: mov r0, 42
  ret i32 42
}

define i32 @const_255() {
; CHECK-LABEL: const_255:
; CHECK: mov r0, 255
  ret i32 255
}

define i32 @const_neg1() {
; CHECK-LABEL: const_neg1:
; CHECK: mov r0, -1
  ret i32 -1
}

define i32 @const_neg128() {
; CHECK-LABEL: const_neg128:
; CHECK: mov r0, -128
  ret i32 -128
}

define i32 @const_neg256() {
; CHECK-LABEL: const_neg256:
; CHECK: mov r0, -256
  ret i32 -256
}

; === Limm constants (outside shimm9 range) ===

define i32 @const_256() {
; CHECK-LABEL: const_256:
; CHECK: or r0, r{{[0-9]+}}, 256
  ret i32 256
}

define i32 @const_1000() {
; CHECK-LABEL: const_1000:
; CHECK: or r0, r{{[0-9]+}}, 1000
  ret i32 1000
}

define i32 @const_100000() {
; CHECK-LABEL: const_100000:
; CHECK: or r0, r{{[0-9]+}}, 100000
  ret i32 100000
}

define i32 @const_0xDEADBEEF() {
; CHECK-LABEL: const_0xDEADBEEF:
; CHECK: or r0,
  ret i32 -559038737
}

; === Void return ===

define void @ret_void() {
; CHECK-LABEL: ret_void:
; CHECK: j [blink]
  ret void
}
