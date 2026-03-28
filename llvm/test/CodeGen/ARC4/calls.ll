; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Test function calls including external calls and libcalls.

declare i32 @external(i32)

define i32 @call_external(i32 %a) {
; CHECK-LABEL: call_external:
; CHECK: jl external
; CHECK: j [blink]
  %r = call i32 @external(i32 %a)
  ret i32 %r
}

define i32 @multiply(i32 %a, i32 %b) {
; CHECK-LABEL: multiply:
; CHECK: jl __mulsi3
; CHECK: j [blink]
  %r = mul i32 %a, %b
  ret i32 %r
}

define i32 @divide(i32 %a, i32 %b) {
; CHECK-LABEL: divide:
; CHECK: jl __divsi3
; CHECK: j [blink]
  %r = sdiv i32 %a, %b
  ret i32 %r
}
