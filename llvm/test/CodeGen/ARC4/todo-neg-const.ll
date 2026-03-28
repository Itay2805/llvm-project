; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Negative constants should be materialized using shimm (mov alias).

; CHECK-LABEL: neg1:
; CHECK: mov r0, -1
; CHECK: j [blink]
define i32 @neg1() {
  ret i32 -1
}
