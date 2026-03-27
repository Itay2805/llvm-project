; RUN: llc -mtriple=arc4 -O2 < %s | FileCheck %s

; Constant materialization uses MOV pseudo (AND with matching shimm/limm)
; Per spec page 108: MOV a,shimm = AND a,shimm,shimm

define i32 @return_small_const() {
; CHECK-LABEL: return_small_const:
; CHECK: mov r0, 42
; CHECK-NEXT: j [r31]
  ret i32 42
}

define i32 @return_large_const() {
; CHECK-LABEL: return_large_const:
; CHECK: mov r0, 100000
; CHECK-NEXT: j [r31]
  ret i32 100000
}

define i32 @return_neg_const() {
; CHECK-LABEL: return_neg_const:
; CHECK: mov r0, -1
; CHECK-NEXT: j [r31]
  ret i32 -1
}

define i32 @return_zero() {
; CHECK-LABEL: return_zero:
; CHECK: mov r0, 0
; CHECK-NEXT: j [r31]
  ret i32 0
}
