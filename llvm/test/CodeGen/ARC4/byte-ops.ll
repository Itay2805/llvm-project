; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; === Byte (i8) operations ===

define i8 @load_i8(ptr %p) {
; CHECK-LABEL: load_i8:
; CHECK: ldb r0, [r0]
  %v = load i8, ptr %p
  ret i8 %v
}

define void @store_i8(ptr %p, i8 %v) {
; CHECK-LABEL: store_i8:
; CHECK: stb r1, [r0]
  store i8 %v, ptr %p
  ret void
}

; === Halfword (i16) operations ===

define i16 @load_i16(ptr %p) {
; CHECK-LABEL: load_i16:
; CHECK: ldw r0, [r0]
  %v = load i16, ptr %p
  ret i16 %v
}

define void @store_i16(ptr %p, i16 %v) {
; CHECK-LABEL: store_i16:
; CHECK: stw r1, [r0]
  store i16 %v, ptr %p
  ret void
}

; === Zero-extend loads ===

define i32 @zextload_i8(ptr %p) {
; CHECK-LABEL: zextload_i8:
; CHECK: ldb r0, [r0]
  %v = load i8, ptr %p
  %z = zext i8 %v to i32
  ret i32 %z
}

define i32 @zextload_i16(ptr %p) {
; CHECK-LABEL: zextload_i16:
; CHECK: ldw r0, [r0]
  %v = load i16, ptr %p
  %z = zext i16 %v to i32
  ret i32 %z
}

; === Sign-extend loads ===

define i32 @sextload_i8(ptr %p) {
; CHECK-LABEL: sextload_i8:
; CHECK: ldb.x r0, [r0
  %v = load i8, ptr %p
  %s = sext i8 %v to i32
  ret i32 %s
}

define i32 @sextload_i16(ptr %p) {
; CHECK-LABEL: sextload_i16:
; CHECK: ldw.x r0, [r0
  %v = load i16, ptr %p
  %s = sext i16 %v to i32
  ret i32 %s
}

; === Truncating stores ===

define void @trunc_store_i8(ptr %p, i32 %v) {
; CHECK-LABEL: trunc_store_i8:
; CHECK: stb r1, [r0]
  %t = trunc i32 %v to i8
  store i8 %t, ptr %p
  ret void
}

define void @trunc_store_i16(ptr %p, i32 %v) {
; CHECK-LABEL: trunc_store_i16:
; CHECK: stw r1, [r0]
  %t = trunc i32 %v to i16
  store i16 %t, ptr %p
  ret void
}
