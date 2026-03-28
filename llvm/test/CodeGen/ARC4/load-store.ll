; RUN: llc -march=arc4 < %s | FileCheck %s

; === Word (i32) load/store ===

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

; === Load with GEP offset ===

define i32 @load_offset_8(ptr %p) {
; CHECK-LABEL: load_offset_8:
; CHECK: ld r0,
  %q = getelementptr i32, ptr %p, i32 2
  %v = load i32, ptr %q
  ret i32 %v
}

define void @store_offset(ptr %p, i32 %v) {
; CHECK-LABEL: store_offset:
; CHECK: st r1,
  %q = getelementptr i32, ptr %p, i32 1
  store i32 %v, ptr %q
  ret void
}

; === Global variable load/store ===

@g = global i32 0

define i32 @load_global() {
; CHECK-LABEL: load_global:
; CHECK: ld r0, [g
  %v = load i32, ptr @g
  ret i32 %v
}

define void @store_global(i32 %v) {
; CHECK-LABEL: store_global:
; CHECK: st r0,
  store i32 %v, ptr @g
  ret void
}

; === Load and use ===

define i32 @load_add(ptr %p, i32 %b) {
; CHECK-LABEL: load_add:
; CHECK: ld r{{[0-9]+}}, [r0]
; CHECK: add r0,
  %v = load i32, ptr %p
  %r = add i32 %v, %b
  ret i32 %r
}

; === Store then load (same address) ===

define i32 @store_load(ptr %p, i32 %v) {
; CHECK-LABEL: store_load:
; CHECK: st r1, [r0]
; CHECK: mov r0, r1
  store i32 %v, ptr %p
  %r = load i32, ptr %p
  ret i32 %r
}

; === Multiple loads ===

define i32 @load_two(ptr %p, ptr %q) {
; CHECK-LABEL: load_two:
; CHECK: ld r{{[0-9]+}},
; CHECK: ld r{{[0-9]+}},
; CHECK: add r0,
  %a = load i32, ptr %p
  %b = load i32, ptr %q
  %c = add i32 %a, %b
  ret i32 %c
}
