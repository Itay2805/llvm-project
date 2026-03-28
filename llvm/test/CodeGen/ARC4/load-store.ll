; RUN: llc -march=arc4 < %s | FileCheck %s

; Load and store operations

define i32 @load_i32(ptr %p) {
; CHECK-LABEL: load_i32:
; CHECK: ld r0, [r0]
  %v = load i32, ptr %p
  ret i32 %v
}

define void @store_i32(ptr %p, i32 %v) {
; CHECK-LABEL: store_i32:
; CHECK: st r1, [r0]
  store i32 %v, ptr %p
  ret void
}

define i32 @load_offset(ptr %p) {
; CHECK-LABEL: load_offset:
; CHECK: ld r0,
  %q = getelementptr i32, ptr %p, i32 2
  %v = load i32, ptr %q
  ret i32 %v
}

@g = global i32 0

define i32 @load_global() {
; CHECK-LABEL: load_global:
; CHECK: ld r0, [g
  %v = load i32, ptr @g
  ret i32 %v
}
