; RUN: not llc -march=arc4 < %s 2>&1 | FileCheck %s
; XFAIL: *

; TODO: Byte/halfword load/store not yet implemented.
; These need ldb/ldw/stb/stw DAG patterns.

; CHECK: Cannot select
define i8 @load_byte(ptr %p) {
  %v = load i8, ptr %p
  ret i8 %v
}
