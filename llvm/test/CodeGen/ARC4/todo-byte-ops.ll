; RUN: llc -march=arc4 < %s -filetype=asm -o - | FileCheck %s

; Byte/halfword load/store use ldb/ldw/stb/stw instructions.

; CHECK-LABEL: load_byte:
; CHECK: ldb r0, [r0]
; CHECK: j [blink]
define i8 @load_byte(ptr %p) {
  %v = load i8, ptr %p
  ret i8 %v
}
