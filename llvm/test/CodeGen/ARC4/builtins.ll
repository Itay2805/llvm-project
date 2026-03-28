; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Verify that all compiler-rt builtins are actually called by the backend.

;; --- 32-bit arithmetic ---

define i32 @test_mul(i32 %a, i32 %b) {
; CHECK-LABEL: test_mul:
; CHECK: jl __mulsi3
  %r = mul i32 %a, %b
  ret i32 %r
}

define i32 @test_sdiv(i32 %a, i32 %b) {
; CHECK-LABEL: test_sdiv:
; CHECK: jl __divsi3
  %r = sdiv i32 %a, %b
  ret i32 %r
}

define i32 @test_udiv(i32 %a, i32 %b) {
; CHECK-LABEL: test_udiv:
; CHECK: jl __udivsi3
  %r = udiv i32 %a, %b
  ret i32 %r
}

define i32 @test_srem(i32 %a, i32 %b) {
; CHECK-LABEL: test_srem:
; CHECK: jl __modsi3
  %r = srem i32 %a, %b
  ret i32 %r
}

define i32 @test_urem(i32 %a, i32 %b) {
; CHECK-LABEL: test_urem:
; CHECK: jl __umodsi3
  %r = urem i32 %a, %b
  ret i32 %r
}

;; --- 32-bit shifts (variable amount → libcall) ---

define i32 @test_shl(i32 %a, i32 %b) {
; CHECK-LABEL: test_shl:
; CHECK: jl __ashlsi3
  %r = shl i32 %a, %b
  ret i32 %r
}

define i32 @test_ashr(i32 %a, i32 %b) {
; CHECK-LABEL: test_ashr:
; CHECK: jl __ashrsi3
  %r = ashr i32 %a, %b
  ret i32 %r
}

define i32 @test_lshr(i32 %a, i32 %b) {
; CHECK-LABEL: test_lshr:
; CHECK: jl __lshrsi3
  %r = lshr i32 %a, %b
  ret i32 %r
}

;; --- 64-bit arithmetic ---

define i64 @test_mul64(i64 %a, i64 %b) {
; CHECK-LABEL: test_mul64:
; CHECK: jl __muldi3
  %r = mul i64 %a, %b
  ret i64 %r
}

define i64 @test_sdiv64(i64 %a, i64 %b) {
; CHECK-LABEL: test_sdiv64:
; CHECK: jl __divdi3
  %r = sdiv i64 %a, %b
  ret i64 %r
}

define i64 @test_udiv64(i64 %a, i64 %b) {
; CHECK-LABEL: test_udiv64:
; CHECK: jl __udivdi3
  %r = udiv i64 %a, %b
  ret i64 %r
}
