; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Large constants (outside shimm9 range) use limm materialization.

; CHECK-LABEL: big_const:
; CHECK: mov r{{[0-9]+}}, 0
; CHECK: or r0, r{{[0-9]+}}, 100000
; CHECK: j [blink]
define i32 @big_const() {
  ret i32 100000
}
